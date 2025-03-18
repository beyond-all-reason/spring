/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StartScriptGen.h"

#include "AIScriptHandler.h"
#include "FileSystem/ArchiveNameResolver.h"
#include "System/TdfParser.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"


namespace StartScriptGen {

//////////////////////////////////////////////////////////////////////////////
//
//  Interface
//


/**
	* helper function that covers the fields that need to be set for every minimal or default startup-script
	* 
	* @param game resolved name of the game
	* @param map resolved name of the map
	* @return a minimal config section containing general required fields
	*/
void CreateMinimalSetupSections(TdfParser::TdfSection& setup, const std::string& map, const std::string& game) {
	const std::string playername = configHandler->GetString("name");
	TdfParser::TdfSection* g = setup.construct_subsection("GAME");
	g->add_name_value("Mapname", map);
	g->add_name_value("Gametype", game);

	g->AddPair("IsHost", 1);
	g->add_name_value("MyPlayerName", playername);

	TdfParser::TdfSection* player0 = g->construct_subsection("PLAYER0");
	player0->add_name_value("Name", playername);
	player0->AddPair("Team", 0);

	TdfParser::TdfSection* team0 = g->construct_subsection("TEAM0");
	team0->AddPair("TeamLeader", 0);
	team0->AddPair("AllyTeam", 0);

	TdfParser::TdfSection* ally0 = g->construct_subsection("ALLYTEAM0");
	ally0->AddPair("NumAllies", 0);
}

std::string CreateMinimalSetup(const std::string& game, const std::string& map)
{
	TdfParser::TdfSection setup;
	CreateMinimalSetupSections(setup, ArchiveNameResolver::GetMap(map), ArchiveNameResolver::GetGame(game));
	// section already present, using this method to get acces to GAME section
	TdfParser::TdfSection* g = setup.construct_subsection("GAME");
	TdfParser::TdfSection* modopts = g->construct_subsection("MODOPTIONS");
	modopts->AddPair("MinimalSetup", 1); //use for ingame detecting this type of start

	std::ostringstream str;
	setup.print(str);
	LOG_L(L_DEBUG, "%s", str.str().c_str());
	return str.str();
}


std::string CreateDefaultSetup(const std::string& map, const std::string& game, const std::string& ai,
			const std::string& playername)
{
	TdfParser::TdfSection setup;
	CreateMinimalSetupSections(setup, map, game);
	// section already present, using this method to get acces to GAME section
	TdfParser::TdfSection* g = setup.construct_subsection("GAME");
	

	const bool isSkirmishAITestScript = CAIScriptHandler::Instance().IsSkirmishAITestScript(ai);
	if (isSkirmishAITestScript) {
		const SkirmishAIData& aiData = CAIScriptHandler::Instance().GetSkirmishAIData(ai);
		TdfParser::TdfSection* ai = g->construct_subsection("AI0");
		ai->add_name_value("Name", "Enemy");
		ai->add_name_value("ShortName", aiData.shortName);
		ai->add_name_value("Version", aiData.version);
		ai->AddPair("Host", 0);
		ai->AddPair("Team", 1);
	} else if (!ai.empty()) { // is no native ai, try lua ai
		TdfParser::TdfSection* aisec = g->construct_subsection("AI0");
		aisec->add_name_value("Name", "AI: " + ai);
		aisec->add_name_value("ShortName", ai);
		aisec->AddPair("Host", 0);
		aisec->AddPair("Team", 1);
	} else {
		TdfParser::TdfSection* player1 = g->construct_subsection("PLAYER1");
		player1->add_name_value("Name", "Enemy");
		player1->AddPair("Team", 1);
	}

	TdfParser::TdfSection* team1 = g->construct_subsection("TEAM1");
	if (isSkirmishAITestScript || !ai.empty()) {
		team1->AddPair("TeamLeader", 0);
	} else {
		team1->AddPair("TeamLeader", 1);
	}
	team1->AddPair("AllyTeam", 1);

	TdfParser::TdfSection* ally1 = g->construct_subsection("ALLYTEAM1");
	ally1->AddPair("NumAllies", 0);

	std::ostringstream str;
	setup.print(str);

	return str.str();
}

} //namespace StartScriptGen
