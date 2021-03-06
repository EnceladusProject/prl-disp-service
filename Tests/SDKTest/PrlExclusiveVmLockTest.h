/////////////////////////////////////////////////////////////////////////////
///
///	@file PrlExclusiveVmLockTest.h
///
///	This file is the part of parallels public SDK library tests suite.
///	Tests fixture class for testing VM exclusive lock functionality.
///
///	@author sandro
///
/// Copyright (c) 1999-2017, Parallels International GmbH
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
/////////////////////////////////////////////////////////////////////////////

#ifndef PrlExclusiveVmLockTest_H
#define PrlExclusiveVmLockTest_H

#include <QtTest/QtTest>

class PrlExclusiveVmLockTest : public QObject
{

Q_OBJECT

private slots:
	void testVmLockUnlock();
	void testVmLockTryToLockVmTwice();
	void testVmLockTryToUnlockVmFromAnotherSession();
	void testVmLockAutoUnlockingOnSessionClosed();
	void testCheckPermissionsOnVmLockUnlockOperations();
	void testVmLockOnWrongParams();
	void testVmUnlockOnWrongParams();
	void testStartVmThenLockThisThenCloseCurrentSession();
	void testStartVmFromOneSessionAndLockItFromAnotherOne();
};

#endif

