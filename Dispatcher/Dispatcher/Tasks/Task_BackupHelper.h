///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_BackupHelper.h
///
/// Common functions for backup tasks
///
/// @author krasnov@
///
/// Copyright (c) 2005-2017, Parallels International GmbH
///
/// This file is part of Virtuozzo Core. Virtuozzo Core is free
/// software; you can redistribute it and/or modify it under the terms
/// of the GNU General Public License as published by the Free Software
/// Foundation; either version 2 of the License, or (at your option) any
/// later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.
///
/// You should have received a copy of the GNU General Public License
/// along with this program; if not, write to the Free Software
/// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
/// 02110-1301, USA.
///
/// Our contact details: Parallels International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////

#ifndef __Task_BackupHelper_H_
#define __Task_BackupHelper_H_

#include <QString>

#include "CDspTaskHelper.h"
#include "Task_DispToDispConnHelper.h"
#include "CDspClient.h"
#include "prlcommon/IOService/IOCommunication/IOClient.h"
#include "prlcommon/VirtualDisk/Qcow2Disk.h"
#include "Libraries/VmFileList/CVmFileListCopy.h"
#include "Libraries/DispToDispProtocols/CVmBackupProto.h"
#include "prlxmlmodel/BackupTree/VmItem.h"
#include "Dispatcher/Dispatcher/CDspDispConnection.h"
#include "CDspVmBackupInfrastructure.h"
#include "CDspVmBackupInfrastructure_p.h"

#define GUEST_FS_SUSPEND_TIMEOUT_SEC 300  // 5 minutes
#define GUEST_FS_SUSPEND_TIMEOUT_WAIT_MSEC 10  // 10 miliseconds

#define PRL_BASE_BACKUP_DIRECTORY	"base"
#define PRL_BASE_BACKUP_NUMBER		1
#define PRL_PARTIAL_BACKUP_START_NUMBER	2 /* as in acronis */
#ifdef _WIN_
#define PRL_ABACKUP_CLIENT "prl_backup_client.exe"
#define PRL_ABACKUP_SERVER "prl_backup_server.exe"
#endif
#ifdef _MAC_
#define PRL_ABACKUP_CLIENT "prl_backup_client"
#define PRL_ABACKUP_SERVER "prl_backup_server"
#endif
#ifdef _LIN_
#define PRL_ABACKUP_CLIENT "/usr/libexec/prl_backup_client"
#define PRL_ABACKUP_SERVER "/usr/libexec/prl_backup_server"
#define VZ_BACKUP_CLIENT "/usr/libexec/vz_backup_client"
#endif

#define PRL_CT_BACKUP_TIB_FILE_NAME "private.tib"

// vzwin: zip archive of miscellaneous CT files from private
#define PRL_CT_BACKUP_ZIP_FILE_NAME "private.zip"
// vzwin: milliseconds wait for zip/unzip  misc files in CT private dir
#define ZIP_WORK_TIMEOUT (10*60*1000)

enum BackupCheckMode {
	PRL_BACKUP_CHECK_MODE_READ,
	PRL_BACKUP_CHECK_MODE_WRITE,
};

namespace Backup
{
namespace Process
{
struct Driver;

///////////////////////////////////////////////////////////////////////////////
// struct Unit

struct Unit: QObject
{
	typedef boost::function3<void, const QString&, int, const QString&>
		callback_type;

	Unit(): m_driver()
	{
	}
	explicit Unit(const callback_type& callback_): m_driver(), m_callback(callback_)
	{
	}
	~Unit()
	{
		reset();
	}

	PRL_RESULT start(QStringList args_, int version_);
	PRL_RESULT waitForFinished();
	void kill();
	PRL_RESULT read(char *buffer, qint32 size, UINT32 tmo = 0);
	PRL_RESULT write(char* data_, quint32 size_);
	PRL_RESULT write(const SmartPtr<char>& data_, quint32 size_);

private slots:
	void reactFinish(int code_, QProcess::ExitStatus status_);

private:
	Q_OBJECT

	void reset();
	static Prl::Expected<QPair<char*, qint32>, PRL_RESULT>
		read_(const QSharedPointer<QLocalSocket>& channel_, char* buffer_, qint32 capacity_);

	QMutex m_mutex;
	Driver* m_driver;
	QString m_program;
	QWaitCondition m_event;
	callback_type m_callback;
	QSharedPointer<QLocalSocket> m_channel;
};

} // namespace Process

namespace Storage
{
///////////////////////////////////////////////////////////////////////////////
// struct Image

struct Image
{
	explicit Image(const QString& path_) : m_path(path_) {}

	PRL_RESULT build(quint64 size_, const QString& base_);
	void remove() const;
	const QString& getPath() const
	{
		return m_path;
	}

private:
	QString m_path;
};

///////////////////////////////////////////////////////////////////////////////
// struct Nbd

struct Nbd
{
	PRL_RESULT start(const Image& image_, quint32 flags_);
	void stop();
	QString getUrl() const;
	void setExportName(const QString& value_)
	{
		m_exportName = value_;
	}

private:
	QString m_url, m_exportName;
	VirtualDisk::Qcow2 m_nbd;
};

} // namespace Storage
} // namespace Backup

enum _PRL_BACKUP_STEP {
	BACKUP_REGISTER_EX_OP	= (1 << 0),
};

///////////////////////////////////////////////////////////////////////////////
// struct Chain

struct Chain
{
	typedef Backup::Process::Unit process_type;

	virtual ~Chain();

	void next(SmartPtr<Chain> next_)
	{
		m_next = next_;
	}
	virtual PRL_RESULT do_(SmartPtr<IOPackage> request_, process_type& dst_) = 0;
protected:
	PRL_RESULT forward(SmartPtr<IOPackage> request_, process_type& dst_);

private:
	SmartPtr<Chain> m_next;
};

///////////////////////////////////////////////////////////////////////////////
// struct Forward

struct Forward: Chain
{
	Forward(SmartPtr<IOClient> client_, quint32 timeout_):
		m_timeout(timeout_), m_client(client_)
	{
	}

	PRL_RESULT do_(SmartPtr<IOPackage> request_, process_type& dst_);
private:
	quint32 m_timeout;
	SmartPtr<IOClient> m_client;
};

class Task_BackupHelper;

namespace Backup
{
///////////////////////////////////////////////////////////////////////////////
// struct Perspective
//

struct Perspective
{
	typedef QList<CVmHardDisk* > imageList_type;
	typedef SmartPtr<CVmConfiguration> config_type;

	explicit Perspective(const config_type& config_);

	bool bad() const
	{
		return !m_config.isValid() || m_config->getVmHardwareList() == NULL;
	}
	imageList_type getImages() const;
	config_type clone(const QString& uuid_, const QString& name_) const;
private:
	static QString getName(const QString& name_, const QStringList& met_);

	config_type m_config;
};

///////////////////////////////////////////////////////////////////////////////
// struct AClient
//

struct AClient
{
	virtual ~AClient()
	{
	}
	virtual PRL_RESULT startABackupClient(const QString&, const QStringList&,
			const QString&, unsigned int) = 0;
	virtual PRL_RESULT startABackupClient(const QString&, const QStringList&,
			SmartPtr<Chain> ) = 0;
};

///////////////////////////////////////////////////////////////////////////////
// struct Suffix

struct Suffix
{
	explicit Suffix(unsigned index_) : m_index(index_) {}
	QString operator()() const;

private:
	unsigned m_index;
};

namespace Work
{
///////////////////////////////////////////////////////////////////////////////
// struct UrlBuilder

struct UrlBuilder
{
	explicit UrlBuilder(const QString& server_) : m_hostname(server_) {}

	QString operator()(const QString& url_) const;
	QString operator()(const ::Backup::Activity::Object::componentList_type& urls_,
			const QString& path_) const;

private:
	QString m_hostname;
};

///////////////////////////////////////////////////////////////////////////////
// struct Ct

struct Ct
{
	explicit Ct(Task_BackupHelper& task_);

	QStringList buildArgs(const Product::component_type& t_,
		const QFileInfo* f_) const;

private:
	Task_BackupHelper *m_context;
};

///////////////////////////////////////////////////////////////////////////////
// struct Vm

struct Vm
{
	explicit Vm(Task_BackupHelper& task_);

	QStringList buildArgs(const QString& snapshot_, const Product::component_type& t_,
			const QFileInfo* f_) const;

private:
	Task_BackupHelper *m_context;
};

typedef boost::variant<Ct, Vm> object_type;

} // namespace Work
} // namespace Backup

///////////////////////////////////////////////////////////////////////////////
// class Task_BackupHelper

class Task_BackupHelper: public CDspTaskHelper, public Task_DispToDispConnHelper,
			protected Backup::AClient, public Toll::Choke
{
	Q_OBJECT

public:
	virtual QString  getVmUuid() { return m_sVmUuid; }

	quint32 getFlags() const
	{
		return m_nFlags;
	}

	quint32 getInternalFlags() const
	{
		return m_nInternalFlags;
	}
	void setInternalFlags(quint32 internalFlags_)
	{
		m_nInternalFlags = internalFlags_;
	}
	bool isRunning() const
	{
		return m_nInternalFlags & PVM_CT_RUNNING;
	}

	const QString& getBackupRoot() const
	{
		return m_sBackupRootPath;
	}
	void setBackupRoot(const QString& backupRoot_)
	{
		m_sBackupRootPath = backupRoot_;
	}

	const QString& getBackupUuid() const
	{
		return m_sBackupUuid;
	}

	unsigned getBackupNumber() const
	{
		return m_nBackupNumber;
	}

	SmartPtr< ::Backup::Product::Model> getProduct() const
	{
		return m_product;
	}

	QString patch(QUrl url_) const;

	Chain * prepareABackupChain(const QStringList& args_, const QString &sNotificationVmUuid,
				unsigned int nDiskIdx);
	PRL_RESULT startABackupClient(const QString& sVmName_, const QStringList& args_,
		const QString &sNotificationVmUuid, unsigned int nDiskIdx);
	PRL_RESULT startABackupClient(const QString& sVmName_, const QStringList& args_,
		SmartPtr<Chain> custom_);
	/* updates an id of the last successful incremental backup */
	PRL_RESULT updateLastPartialNumber(const QString &ve_, const QString &uuid_, unsigned number_);

protected:
	Task_BackupHelper(const SmartPtr<CDspClient> &client, const SmartPtr<IOPackage> &p);
	virtual ~Task_BackupHelper();

	PRL_RESULT connect();

	/* To get directory and file lists for start path with excludes.
	   This function does not clean directory and file lists before.
	   excludeFunc will get relative path and should return 'true' to
           exclude obj from dirList or fileList */
	PRL_RESULT getEntryLists(const QString &sStartPath, bool (*excludeFunc)(const QString &));

	/* find Vm uuid for backup uuid in backup directory */
	PRL_RESULT findVmUuidForBackupUuid(const QString &sBackupUuid, QString &sVmUuid);

	/* get backup directory full path */
	QString getBackupDirectory();

 	/* get last full backup item for vm uuid */
	BackupItem* getLastBaseBackup(const QString &sVmUuid, CAuthHelper *pAuthHelper, BackupCheckMode mode);
 	/* get next partial backup number */
 	unsigned getNextPartialBackup(const QString &sVmUuid, const QString &sBackupUuid);
 	/* parse BackupUuid[.BackupNumber] */
 	PRL_RESULT parseBackupId(const QString &sBackupId, QString &sBackupUuid, unsigned &nBackupNumber);

	PRL_RESULT handleABackupPackage(
			const SmartPtr<CDspDispConnection> &pDispConnection,
			const SmartPtr<IOPackage> &pRequest,
			UINT32 tmo);
	PRL_RESULT GetBackupTreeRequest(const QString &sVmUuid, QString &sBackupTree);
	void killABackupClient();
	PRL_RESULT getBackupParams(const QString &sVmUuid, const QString &sBackupUuid,
		unsigned nBackupNumber, quint64 &nSize, quint32 &nBundlePermissions);
	PRL_RESULT checkFreeDiskSpace(quint64 nRequiredSize, quint64 nAvailableSize, bool bIsCreateOp);

	virtual bool isCancelled() { return operationIsCancelled(); }

	/**
	* reimpl
	*/
	virtual bool providedAdditionState(){return true;}

	/* prepare new DiskDescriptor with linked clone to the original disk
	 * except current delta */
	PRL_RESULT CloneHardDiskState(const QString &sDiskImage,
			const QString &sSnapshotUuid, const QString &sDstDirectory);

	Backup::Metadata::Lock& getMetadataLock();
	Backup::Metadata::Catalog getCatalog(const QString& vm_);

protected:
	SmartPtr<CVmConfiguration> m_pVmConfig;
	QString m_sVmHomePath;
	QString m_sVmName;
	QString m_sVmUuid;
	QString m_sVmDirUuid;
	QString m_sDescription;
	QString m_sServerHostname;
	quint32 m_nServerPort;
	QString m_sServerSessionUuid;
	quint32 m_nFlags;
	quint32 m_nInternalFlags;
	quint32 m_nSteps;
	quint32 m_nBackupTimeout;
	quint32 m_nRemoteVersion;
	QString m_sBackupUuid;
	/* full backup path */
	QString m_sBackupRootPath;
	unsigned m_nBackupNumber;
	quint64 m_nOriginalSize;
	SmartPtr< ::Backup::Product::Model> m_product;
	::Backup::Activity::Service* m_service;

	/* list of directories for plain copy : file info and relative path from Vm home directory */
	QList<QPair<QFileInfo, QString> > m_DirList;
	/* list of files for plain copy : file info and relative path from Vm home directory */
	QList<QPair<QFileInfo, QString> > m_FileList;
	Backup::Process::Unit m_cABackupServer;
	::Backup::Activity::Object::componentList_type m_urls;

private:
	static Backup::Metadata::Lock s_metadataLock;

	Backup::Process::Unit* m_cABackupClient;
	bool m_bKillCalled;
	SmartPtr<char> m_pBuffer;
	qint64 m_nBufSize;
};

#endif //__Task_BackupHelper_H_

