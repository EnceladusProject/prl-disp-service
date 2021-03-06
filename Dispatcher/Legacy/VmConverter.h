///////////////////////////////////////////////////////////////////////////////
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

#ifndef LEGACY_VMCONVERTER_H
#define LEGACY_VMCONVERTER_H

#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include <prlcommon/Std/SmartPtr.h>
#include <prlcommon/PrlCommonUtilsBase/ParallelsDirs.h>
#include <boost/optional.hpp>
#include <prlcommon/PrlCommonUtilsBase/SysError.h>
#include <prlcommon/PrlCommonUtilsBase/ErrorSimple.h>

namespace Legacy
{
namespace Vm
{
typedef Prl::Expected<void, SmartPtr<CVmEvent> > result_type;

///////////////////////////////////////////////////////////////////////////////
// struct V2V

struct V2V
{
	explicit V2V(const CVmConfiguration &cfg): m_cfg(cfg)
	{
	}

	PRL_RESULT do_() const;
	Prl::Expected<void, Error::Simple> start() const;

private:
	CVmConfiguration m_cfg;
};

///////////////////////////////////////////////////////////////////////////////
// struct Converter

struct Converter
{
	result_type convertHardware(SmartPtr<CVmConfiguration> &cfg) const;
	result_type convertBios(const SmartPtr<CVmConfiguration> &cfg) const;
	boost::optional<V2V> getV2V(const CVmConfiguration &cfg) const;
};

} // namespace Vm
} // namespace Legacy

#endif // LEGACY_VMCONVERTER_H
