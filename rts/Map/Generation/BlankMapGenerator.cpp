/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "BlankMapGenerator.h"
#include "Map/SMF/SMFFormat.h"
#include "Map/SMF/SMFReadMap.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/myGL.h"
#include "System/FileSystem/Archives/VirtualArchive.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Exceptions.h"
#include "System/StringUtil.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/Log/ILog.h"

#include <string>
#include <cstring> // strcpy,memset
#include <sstream>

#include <tracy/Tracy.hpp>

CBlankMapGenerator::CBlankMapGenerator(const CGameSetup* setup)
	: setup(setup)
	, mapSize(1, 1)
	, mapHeight(50)
	, mapColor(0x00, 0xFF, 0x00)
{
	const auto& mapOpts = setup->GetMapOptionsCont();

	for (const auto& mapOpt: mapOpts) {
		LOG_L(L_WARNING, "[MapGen::%s] mapOpt<%s,%s>", __func__, mapOpt.first.c_str(), mapOpt.second.c_str());
	}
	for (const auto& modOpt: setup->GetModOptionsCont()) {
		LOG_L(L_WARNING, "[MapGen::%s] modOpt<%s,%s>", __func__, modOpt.first.c_str(), modOpt.second.c_str());
	}

	// `new_map` are legacy keys; see the comment at InitBlank in GameSetup
	const std::string* blankMapXStr = mapOpts.contains("blank_map_x") ? mapOpts.try_get("blank_map_x") : mapOpts.try_get("new_map_x");
	const std::string* blankMapYStr = mapOpts.contains("blank_map_y") ? mapOpts.try_get("blank_map_y") : mapOpts.try_get("new_map_y");
	const std::string* blankMapHeightStr = mapOpts.try_get("blank_map_height");

	if (blankMapXStr != nullptr && blankMapYStr != nullptr) {
		try {
			// mapSize coordinates are actually 2x the spring map dimensions
			// Example: 10x10 map has mapSize = (5, 5)
			const int blankMapX = std::stoi(*blankMapXStr) / 2;
			const int blankMapY = std::stoi(*blankMapYStr) / 2;
		
			if (blankMapX > 0 && blankMapY > 0)
				mapSize = int2(blankMapX, blankMapY);
		
		} catch (...) {
			// leaving default value
		}
	}

	if (blankMapHeightStr != nullptr) {
		try {
			const int blankMapHeight = std::stoi(*blankMapHeightStr);

			mapHeight = blankMapHeight;

		} catch (...) {
			// leaving default value
		}
	}

	const auto blankMapR = mapOpts.try_get("blank_map_color_r");
	const auto blankMapG = mapOpts.try_get("blank_map_color_g");
	const auto blankMapB = mapOpts.try_get("blank_map_color_b");
	if (blankMapR && blankMapG && blankMapB) {
		try {
			std::get <0> (mapColor) = std::stoi(*blankMapR);
			std::get <1> (mapColor) = std::stoi(*blankMapG);
			std::get <2> (mapColor) = std::stoi(*blankMapB);
		} catch (...) { }
	}
}

void CBlankMapGenerator::Generate()
{
	//ZoneScoped;
	// create archive for map
	CVirtualArchive* archive = virtualArchiveFactory->AddArchive(setup->mapName);

	// generate map and fill archive files
	GenerateMap();
	GenerateSMF(archive->GetFilePtr(archive->AddFile("maps/generated.smf")));
	GenerateMapInfo(archive->GetFilePtr(archive->AddFile("mapinfo.lua")));
	GenerateSMT(archive->GetFilePtr(archive->AddFile("maps/generated.smt")));

	// add archive to VFS
	archiveScanner->ScanArchive(setup->mapName + "." + virtualArchiveFactory->GetDefaultExtension());

	// write to disk for testing
	// archive->WriteToFile();
}

void CBlankMapGenerator::GenerateMap()
{
	//ZoneScoped;
	mapDescription = "Blank Map";

	startPositions.emplace_back(20, 20);
	startPositions.emplace_back(500, 500);
}

void CBlankMapGenerator::GenerateSMF(CVirtualFile* fileSMF)
{
	//ZoneScoped;
	SMFHeader smfHeader;
	MapTileHeader smfTile;
	MapFeatureHeader smfFeature;

	//--- Make SMFHeader ---
	std::strcpy(smfHeader.magic, "spring map file");
	smfHeader.version = 1;
	smfHeader.mapid = 0; // just an arbitrary value, could be anything at this point

	//Set settings
	smfHeader.mapx = mapSize.x * CSMFReadMap::bigSquareSize;
	smfHeader.mapy = mapSize.y * CSMFReadMap::bigSquareSize;
	smfHeader.squareSize = 8;
	smfHeader.texelPerSquare = 8;
	smfHeader.tilesize = 32;
	smfHeader.minHeight = (float) mapHeight;
	smfHeader.maxHeight = (float) mapHeight;

	constexpr int32_t numSmallTiles = 1; //2087; //32 * 32 * (mapSize.x  / 2) * (mapSize.y / 2);
	constexpr char smtFileName[] = "generated.smt";

	//--- Extra headers ---
	ExtraHeader vegHeader;
	vegHeader.type = MEH_Vegetation;
	vegHeader.size = sizeof(int);

	smfHeader.numExtraHeaders =  1;

	//Make buffers for each map
	const int32_t heightmapDimensions = (smfHeader.mapx + 1) * (smfHeader.mapy + 1);
	const int32_t typemapDimensions = (smfHeader.mapx / 2) * (smfHeader.mapy / 2);
	const int32_t metalmapDimensions = (smfHeader.mapx / 2) * (smfHeader.mapy / 2);
	const int32_t tilemapDimensions =  (smfHeader.mapx * smfHeader.mapy) / 16;
	const int32_t vegmapDimensions = (smfHeader.mapx / 4) * (smfHeader.mapy / 4);

	const int32_t heightmapSize = heightmapDimensions * sizeof(int16_t);
	const int32_t typemapSize = typemapDimensions * sizeof(uint8_t);
	const int32_t metalmapSize = metalmapDimensions * sizeof(uint8_t);
	const int32_t tilemapSize = tilemapDimensions * sizeof(int32_t);
	const int32_t tilemapTotalSize = sizeof(MapTileHeader) + sizeof(numSmallTiles) + sizeof(smtFileName) + tilemapSize;
	const int32_t vegmapSize = vegmapDimensions * sizeof(uint8_t);

	constexpr int32_t vegmapOffset = sizeof(smfHeader) + sizeof(vegHeader) + sizeof(int32_t);

	std::vector<int16_t> heightmapPtr(heightmapDimensions);
	std::vector<uint8_t> typemapPtr(typemapDimensions);
	std::vector<uint8_t> metalmapPtr(metalmapDimensions);
	std::vector<int32_t> tilemapPtr(tilemapDimensions);
	std::vector<uint8_t> vegmapPtr(vegmapDimensions);

	//--- Set offsets, increment each member with the previous one ---
	smfHeader.heightmapPtr = vegmapOffset + vegmapSize;
	smfHeader.typeMapPtr = smfHeader.heightmapPtr + heightmapSize;
	smfHeader.tilesPtr = smfHeader.typeMapPtr + typemapSize;
	smfHeader.minimapPtr = 0; //smfHeader.tilesPtr + sizeof(MapTileHeader);
	smfHeader.metalmapPtr = smfHeader.tilesPtr + tilemapTotalSize;  //smfHeader.minimapPtr + minimapSize;
	smfHeader.featurePtr = smfHeader.metalmapPtr + metalmapSize;

	//--- Make MapTileHeader ---
	smfTile.numTileFiles = 1;
	smfTile.numTiles = numSmallTiles;

	//--- Make MapFeatureHeader ---
	smfFeature.numFeatures = 0;
	smfFeature.numFeatureType = 0;

	//--- Update Ptrs and write to buffer ---
	std::memset(vegmapPtr.data(), 0, vegmapSize);

	for (int x = 0; x < heightmapDimensions; x++) {
		heightmapPtr[x] = mapHeight;
	}

	std::memset(typemapPtr.data(), 0, typemapSize);

	std::memset(tilemapPtr.data(), 0, tilemapSize);
	std::memset(metalmapPtr.data(), 0, metalmapSize);

	//--- Write to final buffer ---
	AppendToBuffer(fileSMF, smfHeader);

	AppendToBuffer(fileSMF, vegHeader);
	AppendToBuffer(fileSMF, vegmapOffset);
	AppendToBuffer(fileSMF, vegmapPtr.data(), vegmapSize);

	AppendToBuffer(fileSMF, heightmapPtr.data(), heightmapSize);
	AppendToBuffer(fileSMF, typemapPtr.data(), typemapSize);

	AppendToBuffer(fileSMF, smfTile);
	AppendToBuffer(fileSMF, numSmallTiles);
	AppendToBuffer(fileSMF, smtFileName, sizeof(smtFileName));
	AppendToBuffer(fileSMF, tilemapPtr.data(), tilemapSize);

	AppendToBuffer(fileSMF, metalmapPtr.data(), metalmapSize);
	AppendToBuffer(fileSMF, smfFeature);
}

void CBlankMapGenerator::GenerateMapInfo(CVirtualFile* fileMapInfo)
{
	//ZoneScoped;
	//Open template mapinfo.lua
	const std::string luaTemplate = "mapgenerator/mapinfo_template.lua";
	CFileHandler fh(luaTemplate, SPRING_VFS_PWD_ALL);
	if (!fh.FileExists())
		throw content_error("Error generating map: " + luaTemplate + " not found");

	std::string luaInfo;
	fh.LoadStringData(luaInfo);

	//Make info to put in mapinfo
	std::stringstream ss;
	std::string startPosString;
	for (size_t x = 0; x < startPositions.size(); x++) {
		ss << "[" << x << "] = {startPos = {x = " << startPositions[x].x << ", z = " << startPositions[x].y << "}},";
	}
	startPosString = ss.str();

	//Replace tags in mapinfo.lua
	luaInfo = StringReplace(luaInfo, "${NAME}", setup->mapName);
	luaInfo = StringReplace(luaInfo, "${DESCRIPTION}", mapDescription);
	luaInfo = StringReplace(luaInfo, "${START_POSITIONS}", startPosString);

	//Copy to filebuffer
	fileMapInfo->buffer.assign(luaInfo.begin(), luaInfo.end());
}

void CBlankMapGenerator::GenerateSMT(CVirtualFile* fileSMT)
{
	//ZoneScoped;
	constexpr int32_t TILE_SIZE = 32;
	constexpr int32_t TILE_BPP = 3;

	//--- Make TileFileHeader ---
	TileFileHeader smtHeader;
	std::strcpy(smtHeader.magic, "spring tilefile");
	smtHeader.version = 1;
	smtHeader.numTiles = 1; //32 * 32 * (generator->mapSize.x * 32) * (generator->mapSize.y * 32);
	smtHeader.tileSize = TILE_SIZE;
	smtHeader.compressionType = 1;

	uint8_t tileData[TILE_SIZE * TILE_SIZE * 3];
	int32_t tilePos = 0;
	for (int32_t x = 0; x < TILE_SIZE; x++) {
		for (int32_t y = 0; y < TILE_SIZE; y++) {
			tileData[tilePos++] = std::get <0> (mapColor);
			tileData[tilePos++] = std::get <1> (mapColor);
			tileData[tilePos++] = std::get <2> (mapColor);
		}
	}

	glClearErrors("MapGen", __func__, globalRendering->glDebugErrors);
	GLuint tileTex;
	glGenTextures(1, &tileTex);
	glBindTexture(GL_TEXTURE_2D, tileTex);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGB_S3TC_DXT1_EXT, TILE_SIZE, TILE_SIZE, 0, GL_RGB, GL_UNSIGNED_BYTE, tileData);
	glGenerateMipmapEXT(GL_TEXTURE_2D);

	int32_t dxtImageOffset = 0;
	int32_t dxtImageSize = 512;
	int8_t tileDataDXT[SMALL_TILE_SIZE];
	for (int32_t i = 0; i < 4; i++) {
		glGetCompressedTexImage(GL_TEXTURE_2D, i, tileDataDXT + dxtImageOffset);

		dxtImageOffset += dxtImageSize;
		dxtImageSize /= 4;
	}

	glDeleteTextures(1, &tileTex);

	if (glGetError() != GL_NO_ERROR)
		throw content_error("Error generating map - texture generation not supported");

	int32_t writePosition = 0;
	fileSMT->buffer.resize(sizeof smtHeader + smtHeader.numTiles * SMALL_TILE_SIZE);

	std::memcpy(fileSMT->buffer.data() + writePosition, &smtHeader, sizeof smtHeader);
	writePosition += sizeof smtHeader;

	for (int32_t i = 0; i < smtHeader.numTiles; ++i) {
		std::memcpy(fileSMT->buffer.data() + writePosition, tileDataDXT, SMALL_TILE_SIZE);
		writePosition += SMALL_TILE_SIZE;
	}
}

void CBlankMapGenerator::AppendToBuffer(CVirtualFile* file, const void* data, int size)
{
	//ZoneScoped;
	file->buffer.insert(file->buffer.end(), (std::uint8_t*)data, (std::uint8_t*)data + size);
}

void CBlankMapGenerator::SetToBuffer(CVirtualFile* file, const void* data, int size, int position)
{
	//ZoneScoped;
	std::copy((std::uint8_t*)data, (std::uint8_t*)data + size, file->buffer.begin() + position);
}
