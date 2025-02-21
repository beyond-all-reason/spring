/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <string>

namespace StartScriptGen {

	/**
	* creates an almost empty startup-script with game, map and some minimal 1 team-setup variables
	* no error checking is done!
	* the script is saved as _script.txt in the write-directory after the game terminates
	* @param game name of the game
	* @param map name of the map
	* @return the generated startup-script as a string
	*/
	std::string CreateMinimalSetup(const std::string& game, const std::string& map);

	/**
	* creates an almost empty script.txt with game, map and some minimal 2 team-setup variables
	* only few error checking is done!
	* the script is saved as _script.txt in the write-directory after the game terminates
	* @param game resolved name of the game
	* @param map resolved name of the map
	* @param ai ai to use (Lua/Skirmish), if not found "empty" player is used
	* @param playername name to use ingame
	* @return the generated startup-script as a string
	*/
	std::string CreateDefaultSetup(const std::string& map, const std::string& game, const std::string& ai, const std::string& playername);
}
