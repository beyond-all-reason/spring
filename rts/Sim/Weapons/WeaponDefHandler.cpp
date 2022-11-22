/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include <algorithm>
#include <cctype>
#include <iostream>
#include <stdexcept>

#include "WeaponDefHandler.h"
#include "Lua/LuaParser.h"
#include "Sim/Misc/DamageArrayHandler.h"
#include "System/Exceptions.h"
#include "System/StringUtil.h"


static CWeaponDefHandler gWeaponDefHandler;
CWeaponDefHandler* weaponDefHandler = &gWeaponDefHandler;


void CWeaponDefHandler::Init(LuaParser* defsParser)
{
	const LuaTable& rootTable = defsParser->GetRoot().SubTable("WeaponDefs");

	if (!rootTable.IsValid())
		throw content_error("Error loading WeaponDefs");

	std::vector<std::string> weaponNames;
	rootTable.GetKeys(weaponNames);

	weaponDefIDs.reserve(weaponNames.size());
	weaponDefsVector.reserve(weaponNames.size() + 1); // WeaponDef ID's start with 1
	weaponDefsVector.emplace_back();

	for (int nid = 0; nid < weaponNames.size(); nid++) {
		const int wid = nid + 1;
		const std::string& name = weaponNames[nid];
		const LuaTable wdTable = rootTable.SubTable(name);
		weaponDefsVector.emplace_back(wdTable, name, wid);
		weaponDefIDs[name] = wid;
	}
}



const WeaponDef* CWeaponDefHandler::GetWeaponDef(std::string wdName) const
{
	StringToLowerInPlace(wdName);

	const auto it = weaponDefIDs.find(wdName);

	if (it == weaponDefIDs.end())
		return nullptr;

	return &weaponDefsVector[it->second];
}


const WeaponDef* CWeaponDefHandler::GetWeaponDefByID(int id) const
{
	if (!IsValidWeaponDefID(id))
		return nullptr;

	return &weaponDefsVector[id];
}
