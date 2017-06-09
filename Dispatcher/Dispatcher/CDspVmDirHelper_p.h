////////////////////////////////////////////////////////////////////////////////
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
/// @file
///	CDspVmDirHelper_p.h
///
/// @brief
///	Exclusive tasks internals
///
/// @author shrike
///
/// @date
///	2016-06-24
///
/// @history
///
////////////////////////////////////////////////////////////////////////////////

#ifndef __CDspVmDirHelper_p_H_
#define __CDspVmDirHelper_p_H_

#include <boost/logic/tribool.hpp>

namespace Task
{
namespace Vm
{
namespace Exclusive
{
///////////////////////////////////////////////////////////////////////////////
// struct Unit

struct Unit
{
	Unit(PVE::IDispatcherCommands command_, const IOSender::Handle& session_,
		const QString &taskId_):
		m_taskId(taskId_), m_session(session_), m_command(command_)
	{
	}

	const QString& getTaskId() const
	{
		return m_taskId;
	}
	const IOSender::Handle& getSession() const
	{
		return m_session;
	}
	PVE::IDispatcherCommands getCommand() const
	{
		return m_command;
	}
	bool conflicts(const Unit& party_) const;

protected:
	boost::logic::tribool isReconciled(const Unit& party_) const;

private:
	QString m_taskId;
	IOSender::Handle m_session;
	PVE::IDispatcherCommands m_command;

};

struct Native;
///////////////////////////////////////////////////////////////////////////////
// struct Newcomer

struct Newcomer: Unit
{
	explicit Newcomer(const Unit& unit_): Unit(unit_)
	{
	}

	boost::logic::tribool isReconciled(const Native& native_) const;
};

///////////////////////////////////////////////////////////////////////////////
// struct Native

struct Native: Unit
{
	explicit Native(const Unit& unit_): Unit(unit_)
	{
	}

	bool isReconciled(const Newcomer& newcomer_) const;
};

///////////////////////////////////////////////////////////////////////////////
// struct Event

struct Event
{
	enum
	{
		TIMEOUT = 30000
	};

	explicit Event(QMutex& mutex_);

	void set();
	boost::logic::tribool wait();

private:
	QMutex* m_mutex;
	QSharedPointer<std::pair<bool, QWaitCondition> > m_condition;
};

///////////////////////////////////////////////////////////////////////////////
// struct Conflict

struct Conflict
{
	Conflict(const Unit& running_, const Unit& pending_, const Event& resolved_);

	PRL_RESULT getResult() const;
	PRL_RESULT operator()();

private:
	Unit m_pending;
	Unit m_running;
	Event m_resolved;
};

///////////////////////////////////////////////////////////////////////////////
// struct Gang

struct Gang
{
	explicit Gang(const Event& resolver_): m_resolver(resolver_)
	{
	}

	bool isEmpty() const
	{
		return m_store.isEmpty();
	}
	Conflict* join(const Unit& unit_);
	bool eraseFirst(PVE::IDispatcherCommands command_);
	bool eraseFirst(PVE::IDispatcherCommands command_, const IOSender::Handle& session_);
	const Unit* findFirst(PVE::IDispatcherCommands command_) const;

private:
	typedef QMultiHash<PVE::IDispatcherCommands, Native> store_type;

	Event m_resolver;
	store_type m_store;
};

} // namespace Exclusive
} // namespace Vm
} // namespace Task

#endif // __CDspVmDirHelper_p_H_

