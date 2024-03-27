/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef COB_FILE_H
#define COB_FILE_H

#include <array>
#include <vector>
#include <string>

#include "Lua/LuaHashString.h"
#include "CobScriptNames.h"
#include "System/UnorderedMap.hpp"


//The following structure is taken from http://visualta.tauniverse.com/Downloads/ta-cob-fmt.txt
//Information on missing fields from Format_Cob.pas
typedef struct tagCOBHeader
{
	int VersionSignature; // 4 for ta, 6 for tak, 8 for RAS
	int NumberOfScripts;
	int NumberOfPieces;
	int TotalScriptLen;
	int NumberOfStaticVars;
	int Unknown_2; /* Always seems to be 0 */
	int OffsetToScriptCodeIndexArray;
	int OffsetToScriptNameOffsetArray;
	int OffsetToPieceNameOffsetArray;
	int OffsetToScriptCode;
	int Unknown_3; /* Always seems to point to first script name */

	int OffsetToSoundNameArray;		// These two are only found in TA:K scripts
	int NumberOfSounds;
} COBHeader;

class CFileHandler;

class CCobFile
{
public:
	CCobFile(CFileHandler& in, const std::string& scriptName);
	CCobFile(CCobFile&& f) { *this = std::move(f); }

	CCobFile& operator = (CCobFile&& f) {
		numStaticVars = f.numStaticVars;
		cobVersion = f.cobVersion;

		code = std::move(f.code);
		scriptNames = std::move(f.scriptNames);
		scriptOffsets = std::move(f.scriptOffsets);

		scriptLengths = std::move(f.scriptLengths);
		pieceNames = std::move(f.pieceNames);
		scriptIndex = std::move(f.scriptIndex);
		sounds = std::move(f.sounds);
		luaScripts = std::move(f.luaScripts);
		scriptMap = std::move(f.scriptMap);

		name = std::move(f.name);
		return *this;
	}

	//void LogHeader(const char * calledFrom){
	//	const char *cname = name.c_str();
	//	LOG_L(L_ERROR, "[LogHeader::%s %p] %s %i %i %i %i %i", cname, this, calledFrom, ch.VersionSignature, ch.NumberOfScripts, ch.NumberOfPieces, ch.TotalScriptLen, ch.NumberOfStaticVars);
	//}

	int GetFunctionId(const std::string& name);

public:
	int numStaticVars = 0;
	int cobVersion = 0;

	std::vector<int> code;
	std::vector<std::string> scriptNames;
	std::vector<int> scriptOffsets;
	/// Assumes that the scripts are sorted by offset in the file
	std::vector<int> scriptLengths;
	std::vector<std::string> pieceNames;
	std::array<int, COBFN_NumUnitFuncs> scriptIndex;
	std::vector<int> sounds;
	std::vector<LuaHashString> luaScripts;
	spring::unordered_map<std::string, int> scriptMap;

	std::string name;
};

#endif // COB_FILE_H

