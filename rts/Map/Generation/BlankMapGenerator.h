/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _BLANK_MAP_GENERATOR_H_
#define _BLANK_MAP_GENERATOR_H_

#include "Game/GameSetup.h"
#include "System/type2.h"

class CVirtualFile;
class CVirtualArchive;

class CBlankMapGenerator
{
public:
	CBlankMapGenerator(const CGameSetup* setup);

	void Generate();

private:
	void GenerateMap();
	void GenerateSMF(CVirtualFile*);
	void GenerateMapInfo(CVirtualFile*);
	void GenerateSMT(CVirtualFile*);

	template<typename T>
	void AppendToBuffer(CVirtualFile* file, const T& data) { AppendToBuffer(file, &data, sizeof(T)); }

	template<typename T>
	void SetToBuffer(CVirtualFile* file, const T& data, int position) { SetToBuffer(file, &data, sizeof(T), position); }

	void AppendToBuffer(CVirtualFile* file, const void* data, int size);
	void SetToBuffer(CVirtualFile* file, const void* data, int size, int position);

	const CGameSetup* const setup;

	int2 mapSize;
	int mapHeight;
	std::string mapDescription;
	std::vector<int2> startPositions;
};

#endif // _BLANK_MAP_GENERATOR_H_
