///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_BackupHelper.h
///
/// Common functions for backup tasks
///
/// @author krasnov@
///
/// Copyright (c) 2005-2015 Parallels IP Holdings GmbH
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
/// Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
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
#include "Libraries/VmFileList/CVmFileListCopy.h"
#include "prlxmlmodel/BackupTree/VmItem.h"
#include "Dispatcher/Dispatcher/CDspDispConnection.h"

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
#define PRL_ABACKUP_CLIENT "/usr/sbin/prl_backup_client"
#define PRL_ABACKUP_SERVER "/usr/sbin/prl_backup_server"
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

class BackupProcess
{
public:
	friend class Task_BackupHelper;

	BackupProcess();
	virtual ~BackupProcess();
	PRL_RESULT start(QObject *pParent, QString cmd, QStringList args, UINT32 tmo);
	PRL_RESULT waitForFinished();
	void kill();
	PRL_RESULT readFromABackupClient(char *buffer, qint32 size);
	PRL_RESULT readFromABackupServer(char *buffer, qint32 size, UINT32 tmo);
	PRL_RESULT writeToABackupClient(char *buffer, quint32 size, UINT32 tmo);
	PRL_RESULT writeToABackupServer(char *buffer, quint32 size, UINT32 tmo);
	PRL_RESULT handleABackupPackage(
			const SmartPtr<CDspDispConnection> &pDispConnection,
			const SmartPtr<IOPackage> &pRequest);
	int getExitCode() { return m_nRetCode; }
	void setInFdNonBlock();

private:
	QString m_sCmd;
	int m_nRetCode;
#ifdef _WIN_
	HANDLE m_in;
	HANDLE m_out;
	HANDLE m_process;
	bool m_isKilled;
#else
	pid_t m_pid;
	bool m_isKilled;
	int m_in;
	int m_out;
#endif
};

enum _PRL_BACKUP_STEP {
	BACKUP_REGISTER_EX_OP	= (1 << 0),
	BACKUP_LOCKED_CTID	= (1 << 1),
};

///////////////////////////////////////////////////////////////////////////////
// struct Chain

struct Chain
{
	virtual ~Chain();

	void next(SmartPtr<Chain> next_)
	{
		m_next = next_;
	}
	virtual PRL_RESULT do_(SmartPtr<IOPackage> request_, BackupProcess& dst_) = 0;
protected:
	PRL_RESULT forward(SmartPtr<IOPackage> request_, BackupProcess& dst_);

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

	PRL_RESULT do_(SmartPtr<IOPackage> request_, BackupProcess& dst_);
private:
	quint32 m_timeout;
	SmartPtr<IOClient> m_client;
};

namespace Backup
{
///////////////////////////////////////////////////////////////////////////////
// struct Archive
//

struct Archive
{
	Archive(CVmHardDisk* device_, const QString& name_,
		const QString& image_):
		m_device(device_), m_name(name_), m_image(image_)
	{
	}
	Archive(CVmHardDisk* device_, const QString& name_,
		const QString& image_, const QString& home_);

	const QString& getName() const
	{
		return m_name;
	}
	const CVmHardDisk& getDevice() const
	{
		return *m_device;
	}
	const QString& getImageFolder() const
	{
		return m_image;
	}
	QString getPath(const QString& prefix_) const;
	QString getRestoreFolder() const;
	QString getSnapshotFolder(const QString& prefix_) const;
private:
	CVmHardDisk* m_device;
	QString m_name;
	QString m_home;
	QString m_image;
};

///////////////////////////////////////////////////////////////////////////////
// struct Perspective
//

struct Perspective
{
	typedef QList<CVmHardDisk* > imageList_type;
	typedef QList<Archive> archiveList_type;
	typedef SmartPtr<CVmConfiguration> config_type;

	explicit Perspective(const config_type& config_);

	bool bad() const
	{
		return !m_config.isValid() || m_config->getVmHardwareList() == NULL;
	}
	imageList_type getImages() const;
	archiveList_type getCtArchives(const QString& home_) const;
	archiveList_type getVmArchives(const QString& home_) const;
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
	virtual PRL_RESULT startABackupClient(QString , QStringList , CVmEvent* ,
			const QString& , unsigned int , bool , UINT32 ) = 0;
	virtual PRL_RESULT startABackupClient(QString , QStringList , CVmEvent* ,
			UINT32 , SmartPtr<Chain> ) = 0;
};

} // namespace Backup

///////////////////////////////////////////////////////////////////////////////
// class Task_BackupHelper

class Task_BackupHelper: public CDspTaskHelper, public Task_DispToDispConnHelper,
			protected Backup::AClient
{
	Q_OBJECT

public:
	virtual QString  getVmUuid() { return m_sVmUuid; }

protected:
	Task_BackupHelper(const SmartPtr<CDspClient> &client, const SmartPtr<IOPackage> &p);
	virtual ~Task_BackupHelper();

	PRL_RESULT connect();

	/* To get directory and file lists for start path with excludes.
	   This function does not clean directory and file lists before.
	   excludeFunc will get relative path and should return 'true' to
           exclude obj from dirList or fileList */
	PRL_RESULT getEntryLists(const QString &sStartPath, bool (*excludeFunc)(const QString &));

	/* load data from .metadata file for Vm */
	PRL_RESULT loadVmMetadata(const QString &sVmUuid, VmItem *pVmItem);

	/* load data from .metadata file for base backup */
	PRL_RESULT loadBaseBackupMetadata(
			const QString &sVmUuid,
			const QString &sBackupUuid,
			BackupItem *pBackupItem);

	/* load data from .metadata file for partial backup */
	PRL_RESULT loadPartialBackupMetadata(
			const QString &sVmUuid,
			const QString &sBackupUuid,
			unsigned nBackupNumber,
			PartialBackupItem *pPartialBackupItem);

	/* find Vm uuid for backup uuid in backup directory */
	PRL_RESULT findVmUuidForBackupUuid(const QString &sBackupUuid, QString &sVmUuid);

	/* get backup directory full path */
	QString getBackupDirectory();

	PRL_RESULT lockShared(const QString &sBackupUuid);

	PRL_RESULT lockExclusive(const QString &sBackupUuid);

	void unlockShared(const QString &sBackupUuid);

	void unlockExclusive(const QString &sBackupUuid);

	/* get full backups uuid list for vm uuid */
 	void getBaseBackupList(
				const QString &sVmUuid,
				QStringList &lstBackupUuid,
				CAuthHelper *pAuthHelper = NULL,
				BackupCheckMode mode = PRL_BACKUP_CHECK_MODE_READ);
 	/* get last full backup item for vm uuid */
	BackupItem* getLastBaseBackup(const QString &sVmUuid, CAuthHelper *pAuthHelper, BackupCheckMode mode);
 	/* get partial backups list for vm uuid and full backup uuid */
 	void getPartialBackupList(
 				const QString &sVmUuid,
 				const QString &sBackupUuid,
 				QList<unsigned> &lstBackupNumber);
 	/* get next partial backup number */
 	unsigned getNextPartialBackup(const QString &sVmUuid, const QString &sBackupUuid);
 	/* parse BackupUuid[.BackupNumber] */
 	PRL_RESULT parseBackupId(const QString &sBackupId, QString &sBackupUuid, unsigned &nBackupNumber);

 	/* is backup directory for sBackupUuid is empty from base and partial backups */
 	bool isBackupDirEmpty(const QString &sVmUuid, const QString &sBackupUuid);

	PRL_RESULT startABackupClient(QString sVmName, QStringList args, CVmEvent *pEvent,
		const QString &sNotificationVmUuid, unsigned int nDiskIdx, bool bLocalMode, UINT32 tmo);
	PRL_RESULT startABackupClient(QString sVmName, QStringList args, CVmEvent *pEvent,
			UINT32 tmo, SmartPtr<Chain> custom_);
	PRL_RESULT handleABackupPackage(
			const SmartPtr<CDspDispConnection> &pDispConnection,
			const SmartPtr<IOPackage> &pRequest,
			UINT32 tmo);
	PRL_RESULT GetBackupTreeRequest(const QString &sVmUuid, QString &sBackupTree);
	QString getAcronisErrorString(int code);
	void killABackupClient();
	PRL_RESULT getBackupParams(const QString &sVmUuid, const QString &sBackupUuid,
                unsigned nBackupNumber, quint64 &nSize, quint32 &nBundlePermissions);
	PRL_RESULT checkFreeDiskSpace(const QString &sVmUuid,
			quint64 nRequiredSize, quint64 nAvailableSize, bool bIsCreateOp);

	virtual bool isCancelled() { return operationIsCancelled(); }

	/**
	* reimpl
	*/
	virtual bool providedAdditionState(){return true;}

	PRL_RESULT handleCtPrivateBeforeBackup(
			const QString& sVmUuid,
			const QString& sCtHomePath,
			const QString& sBackupUuid,
			QString& sVzCachePath);
	void cleanupPrivateArea();

	/* prepare new DiskDescriptor with linked clone to the original disk
	 * except current delta */
	PRL_RESULT CloneHardDiskState(const QString &sDiskImage,
			const QString &sSnapshotUuid, const QString &sDstDirectory);

	PRL_RESULT loadVeConfig(const QString &backupUuid, const QString &path,
		PRL_VM_BACKUP_TYPE type, SmartPtr<CVmConfiguration>& conf);

protected:
	QString m_sVmUuid;
	QString m_sServerHostname;
	quint32 m_nServerPort;
	QString m_sServerSessionUuid;
	quint32 m_nFlags;
	quint32 m_nSteps;
	quint32 m_nBackupTimeout;
	quint32 m_nRemoteVersion;

	/* list of directories for plain copy : file info and relative path from Vm home directory */
	QList<QPair<QFileInfo, QString> > m_DirList;
	/* list of files for plain copy : file info and relative path from Vm home directory */
	QList<QPair<QFileInfo, QString> > m_FileList;
	BackupProcess m_cABackupServer;
private:
	BackupProcess* m_cABackupClient;
	bool m_bKillCalled;
	SmartPtr<char> m_pBuffer;
	qint64 m_nBufSize;
	QFile m_filePrivate;
	QFile m_fileCache;
};

#endif //__Task_BackupHelper_H_