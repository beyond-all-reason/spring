#include "SystemGlobalUtils.h"

using namespace SystemGlobals;

SystemGlobals::SystemGlobal SystemGlobals::systemGlobals;

CR_BIND(SystemGlobal, )
CR_REG_METADATA(SystemGlobal, (
	CR_MEMBER(systemGlobalsEntity)
))