/* This file is part of the Recoil engine (GPL v2 or later). */

#include "LuaTracyExtra.h"

#include "LuaInclude.h"
#include "LuaUtils.h"

#include "System/Misc/TracyDefs.h"
#include <common/TracyQueue.hpp>

#include <functional>
#include <set>
#include <string>

/* Tracy seems to want unique, unchanging strings to be passed to
 * its API, so we need to immanentize the ephemeral Lua strings
 * by storing them.
 *
 * NB: strings here are never really cleaned up, but the use case assumes
 * that they live a long time and there's just a handful of them. */
static std::set <std::string, std::less<>> tracyLuaPlots;

static const std::string& GetImmanentPlotName(const char *plotName)
{
	const auto plot = tracyLuaPlots.find(plotName);
	if (plot != tracyLuaPlots.end())
		return *plot;

	return *tracyLuaPlots.emplace(plotName).first;
}

/*** Configure custom appearance for a Tracy plot for use in debugging or profiling
 *
 * @function tracy.LuaTracyPlotConfig
 * @string plotName name of the plot to customize
 * @string[opt="Number"] plotFormatType "Number"|"Percentage"|"Memory"
 * @bool[opt=true] stepwise stepwise chart
 * @bool[opt=false] fill whether to fill color
 * @number[opt=white] color unit32 number as BGR color
 * @treturn nil
 */

static int LuaTracyPlotConfig(lua_State* L)
{
	const auto plotName             = luaL_checkstring(L, 1);
	const auto plotFormatTypeString = luaL_optstring(L, 2, "");
	const auto stepwise             = luaL_optboolean(L, 3, true);
	const auto fill                 = luaL_optboolean(L, 4, false);
	const uint32_t color            = luaL_optint(L, 5, 0xFFFFFF); // white

	tracy::PlotFormatType plotFormatType;
	switch (plotFormatTypeString[0]) {
		case 'p': case 'P': plotFormatType = tracy::PlotFormatType::Percentage; break;
		case 'm': case 'M': plotFormatType = tracy::PlotFormatType::Memory;     break;
		default:            plotFormatType = tracy::PlotFormatType::Number;     break;
	}

	TracyPlotConfig(GetImmanentPlotName(plotName).c_str(), plotFormatType, stepwise, fill, color);
	return 0;
}


/*** Update a Tracy plot with a value
 *
 * @function tracy.LuaTracyPlot
 * @string plotName which LuaPlot should be updated
 * @number plotValue the number to show on the Tracy plot
 * @treturn nil
 */
static int LuaTracyPlot(lua_State* L)
{
	const auto plotName  = luaL_checkstring(L, 1);
	const auto plotValue = luaL_checkfloat (L, 2);

	TracyPlot(GetImmanentPlotName(plotName).c_str(), plotValue);
	return 0;
}

/******************************************************************************
 * tracy extensions
 * @module TracyExtra
 * @see rts/Lua/LuaTracyExtra.cpp
******************************************************************************/

bool LuaTracyExtra::PushEntries(lua_State* L)
{
	LuaPushNamedCFunc(L, "LuaTracyPlot"      , LuaTracyPlot      );
	LuaPushNamedCFunc(L, "LuaTracyPlotConfig", LuaTracyPlotConfig);
	return true;
}
