/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <chrono>
#include <string_view>
#include <fmt/printf.h>

#include "IModelParser.h"
#include "3DOParser.h"
#include "S3OParser.h"
#include "AssParser.h"
#include "3DModelVAO.h"
#include "ModelsLock.h"
#include "Game/GlobalUnsynced.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Net/Protocol/NetProtocol.h" // NETLOG
#include "Sim/Misc/CollisionVolume.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Log/ILog.h"
#include "System/StringUtil.h"
#include "System/Exceptions.h"
#include "System/SafeUtil.h"
#include "System/Threading/ThreadPool.h"
#include "System/ContainerUtil.h"
#include "System/LoadLock.h"
#include "lib/assimp/include/assimp/Importer.hpp"

#include "System/Misc/TracyDefs.h"


CModelLoader modelLoader;

static C3DOParser g3DOParser;
static CS3OParser gS3OParser;
static CAssParser gAssParser;


static bool CheckAssimpWhitelist(const char* aiExt) {
	constexpr std::array<const char*, 5> whitelist = {
		"3ds"  , // 3DSMax
		"dae"  , // Collada
		"lwo"  , // LightWave
		"obj"  ,
		"blend", // Blender
	};

	const auto pred = [&aiExt](const char* wlExt) { return (strcmp(aiExt, wlExt) == 0); };
	const auto iter = std::find_if(whitelist.begin(), whitelist.end(), pred);

	return (iter != whitelist.end());
}

static void RegisterModelFormats(CModelLoader::ParsersType& parsers) {
	RECOIL_DETAILED_TRACY_ZONE;
	// file-extension should be lowercase
	parsers.emplace_back("3do", &g3DOParser);
	parsers.emplace_back("s3o", &gS3OParser);

	std::string extension;
	std::string extensions;
	std::string enabledExtensions;

	Assimp::Importer importer;
	// get a ";" separated list of format extensions ("*.3ds;*.lwo;*.mesh.xml;...")
	importer.GetExtensionList(extensions);
	// do not ignore the last extension
	extensions.append(";");

	size_t curIdx = 0;
	size_t nxtIdx = 0;

	// split the list, strip off the "*." extension prefixes
	while ((nxtIdx = extensions.find(';', curIdx)) != std::string::npos) {
		extension = extensions.substr(curIdx, nxtIdx - curIdx);
		extension = extension.substr(extension.find("*.") + 2);
		extension = StringToLower(extension);

		curIdx = nxtIdx + 1;

		if (!CheckAssimpWhitelist(extension.c_str()))
			continue;
		if (std::find_if(parsers.begin(), parsers.end(), [&extension](const auto& item) { return item.first == extension; }) != parsers.end())
			continue;

		parsers.emplace_back(extension, &gAssParser);
		enabledExtensions.append("*." + extension + ";");
	}

	LOG("[%s] supported (Assimp) model formats: %s", __func__, enabledExtensions.c_str());
}

static void LoadDummyModel(S3DModel& model)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// create a crash-dummy
	model.type = MODELTYPE_3DO;
	model.numPieces = 1;
	// give it one empty piece
	model.AddPiece(g3DOParser.AllocPiece());
	model.FlattenPieceTree(model.GetRootPiece()); //useless except for setting up matAlloc
	model.GetRootPiece()->SetCollisionVolume(CollisionVolume('b', 'z', -UpVector, ZeroVector));
	model.loadStatus = S3DModel::LoadStatus::LOADED;
}

static void LoadDummyModel(S3DModel& model, int id)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// create a crash-dummy
	model.id = id;
	LoadDummyModel(model);
}

static void CheckPieceNormals(const S3DModel* model, const S3DModelPiece* modelPiece)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (auto vertCount = modelPiece->GetVerticesVec().size(); vertCount >= 3) {
		// do not check pseudo-pieces
		unsigned int numNullNormals = 0;

		for (unsigned int n = 0; n < vertCount; n++) {
			numNullNormals += (modelPiece->GetNormal(n).SqLength() < 0.5f);
		}

		if (numNullNormals > 0) {
			constexpr const char* formatStr =
				"[%s] piece \"%s\" of model \"%s\" has %u (of %u) normals with invalid length (<0.5)";

			const char* modelName = model->name.c_str();
			const char* pieceName = modelPiece->name.c_str();

			LOG_L(L_DEBUG, formatStr, __func__, pieceName, modelName, static_cast<uint32_t>(numNullNormals), static_cast<uint32_t>(vertCount));
		}
	}

	for (const S3DModelPiece* childPiece: modelPiece->children) {
		CheckPieceNormals(model, childPiece);
	}
}



void CModelLoader::Init()
{
	RECOIL_DETAILED_TRACY_ZONE;
	RegisterModelFormats(parsers);
	InitParsers();

	models.clear();
	models.resize(MAX_MODEL_OBJECTS);

	// dummy first model, legitimate model IDs start at 1
	modelID = 0;
	LoadDummyModel(models[0], modelID);
}

void CModelLoader::InitParsers() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	g3DOParser.Init();
	gS3OParser.Init();
	gAssParser.Init();
}

void CModelLoader::Kill()
{
	RECOIL_DETAILED_TRACY_ZONE;
	KillModels();
	KillParsers();

	errors.clear();
	cache.clear();
	parsers.clear();
}

void CModelLoader::KillModels()
{
	RECOIL_DETAILED_TRACY_ZONE;
	models.clear();
	modelID = 0;
}

void CModelLoader::KillParsers() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	g3DOParser.Kill();
	gS3OParser.Kill();
	gAssParser.Kill();
}



std::string CModelLoader::FindModelPath(std::string name) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	// check for empty string because we can be called
	// from Lua*Defs and certain features have no models
	if (name.empty())
		return name;

	const std::string vfsPath = "objects3d/";

	if (const std::string& fileExt = FileSystem::GetExtension(name); fileExt.empty()) {
		for (const auto& [formatExt, parser] : parsers) {
			if (CFileHandler::FileExists(name + "." + formatExt, SPRING_VFS_ZIP)) {
				name.append("." + formatExt);
				break;
			}
		}
	}

	if (CFileHandler::FileExists(name, SPRING_VFS_ZIP))
		return name;

	if (name.find(vfsPath) != std::string::npos)
		return name;

	return (FindModelPath(vfsPath + name));
}


void CModelLoader::PreloadModel(const std::string& modelName)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(Threading::IsMainThread() || Threading::IsGameLoadThread());

	//NB: do preload in any case
	if (ThreadPool::HasThreads()) {

		// if already in cache, thread just returns early
		// not spawning the thread at all would be better but still
		// requires locking around cache.find(...) since some other
		// preload worker might be down in FillModel modifying it
		// at the same time
		preloadFutures.emplace_back(
			ThreadPool::Enqueue([modelName]() {
				modelLoader.LoadModel(modelName, true);
			})
		);
	}
	else {
		modelLoader.LoadModel(modelName, true);
	}
}

void CModelLoader::LogErrors()
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(Threading::IsMainThread() || Threading::IsGameLoadThread());

	// block any preload threads from modifying <errors>
	auto lock = CModelsLock::GetScopedLock();
	if (errors.empty())
		return;

	for (const auto& [modelName, reason]: errors) {
		std::string errStr = fmt::sprintf("[FATAL ERROR]: could not load model \"%s\" (reason: %s)", modelName, reason);
		LOG_L(L_ERROR, "%s", errStr.c_str());
		CLIENT_NETLOG(gu->myPlayerNum, LOG_LEVEL_FATAL, errStr.c_str());
	}

	errors.clear();
	throw content_error("[CModelLoader] couldn't load one or many models");
}


S3DModel* CModelLoader::LoadModel(std::string name, bool preload)
{
	ZoneScoped;
	// cannot happen except through SpawnProjectile
	if (name.empty())
		return nullptr;

	StringToLowerInPlace(name);

	bool load = false;
	S3DModel* model = nullptr;
	{
		auto lock = CModelsLock::GetScopedLock();

		model = GetCachedModel(name);

		load = (model->loadStatus == S3DModel::LoadStatus::NOTLOADED);
		if (load)
			model->loadStatus = S3DModel::LoadStatus::LOADING;
	}

	assert(model);
	if (load) {
		FillModel(*model, name, FindModelPath(name));
		cv.notify_all();
	}

	auto lock = CModelsLock::GetUniqueLock();
	cv.wait(lock, [model]() {
		return model->loadStatus == S3DModel::LoadStatus::LOADED;
	});

	if (!preload)
		Upload(model);

	return model;
}

S3DModel* CModelLoader::GetCachedModel(std::string fullName)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// caller has mutex lock

	static const auto CompPred = [](auto&& lhs, auto&& rhs) { return lhs.first < rhs.first; };

	static constexpr std::string_view O3D = "objects3d/";
	if (auto oi = fullName.find(O3D); oi != std::string::npos) {
		assert(oi == 0u);
		fullName.erase(0, O3D.size());
	}

	auto keyFull = std::make_pair(fullName, uint32_t(-1));
	const auto ci = spring::BinarySearch(cache.begin(), cache.end(), keyFull, CompPred);
	if (ci != cache.end()) {
		assert(keyFull.first == ci->first);
		return &models[ci->second];
	}

	auto keyName = std::pair<std::string, uint32_t>("", uint32_t(-1));
	if (const auto ext = FileSystem::GetExtension(fullName); !ext.empty()) {
		keyName.first = fullName.substr(0, fullName.size() - ext.size() - 1);
		const auto ci = spring::BinarySearch(cache.begin(), cache.end(), keyName, CompPred);
		if (ci != cache.end()) {
			assert(keyName.first == ci->first);
			return &models[ci->second];
		}
	}

	if (modelID + 1 == MAX_MODEL_OBJECTS) {
		LOG_L(L_ERROR, "[CModelLoader::%s] Model pool of size %i is exhausted. Cannot load model %s", __func__, MAX_MODEL_OBJECTS, fullName.c_str());
		return &models[0]; //dummy model
	}

	models[modelID].id = ++modelID;
	keyFull.second = models[modelID].id;
	keyName.second = models[modelID].id;

	spring::VectorInsertSorted(cache, std::move(keyFull), CompPred);
	if (!keyName.first.empty())
		spring::VectorInsertSorted(cache, std::move(keyName), CompPred);

	return &models[modelID];
}

void CModelLoader::FillModel(
	S3DModel& model,
	const std::string& name,
	const std::string& path
) {
	ParseModel(model, name, path);

	assert(model.numPieces != 0);
	assert(model.GetRootPiece() != nullptr);

	model.SetPieceMatrices();

	PostProcessGeometry(&model);
}

void CModelLoader::DrainPreloadFutures(uint32_t numAllowed)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (preloadFutures.size() <= numAllowed)
		return;

	const auto erasePredicate = [](decltype(preloadFutures)::value_type item) {
		using namespace std::chrono_literals;
		return item.wait_for(0ms) == std::future_status::ready;
	};

	// collect completed futures
	std::erase_if(preloadFutures, erasePredicate);

	if (preloadFutures.size() <= numAllowed)
		return;

	while (preloadFutures.size() > numAllowed) {
		//drain queue until there are <= numAllowed items there
		std::erase_if(preloadFutures, erasePredicate);
		spring_sleep(spring_msecs(100));
	}
}

IModelParser* CModelLoader::GetFormatParser(const std::string& pathExt)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// cached record
	static std::pair<std::string, IModelParser*> lastParser = {};

	const std::string extension = StringToLower(pathExt);

	if (lastParser.first == extension)
		return lastParser.second;

	const auto it = std::find_if(parsers.begin(), parsers.end(), [&extension](const auto& item) { return item.first == extension; });
	if (it == parsers.end())
		return nullptr;

	lastParser = *it;
	return it->second;
}

void CModelLoader::ParseModel(S3DModel& model, const std::string& name, const std::string& path)
{
	RECOIL_DETAILED_TRACY_ZONE;
	try {
		auto* parser = GetFormatParser(FileSystem::GetExtension(path));
		if (parser == nullptr) {
			LoadDummyModel(model);
			throw content_error(fmt::sprintf("could not find a parser for model \"%s\" (unknown format?)", name));
		}

		parser->Load(model, path);
		if (model.numPieces > MAX_PIECES_PER_MODEL) {
			LoadDummyModel(model);
			throw content_error(fmt::sprintf("A model has too many pieces (>%u): %s", MAX_PIECES_PER_MODEL, path));
		}

	} catch (const content_error& ex) {
		auto lock = CModelsLock::GetScopedLock();
		LoadDummyModel(model);
		errors.emplace_back(name, ex.what());
	}
}



void CModelLoader::PostProcessGeometry(S3DModel* model)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (model->loadStatus == S3DModel::LoadStatus::LOADED)
		return;

	// does quads and strips conversion sometimes. Need to run first
	for (size_t i = 0; i < model->pieceObjects.size(); ++i) {
		auto* p = model->pieceObjects[i];
		p->PostProcessGeometry(static_cast<uint32_t>(i));
		p->CreateShatterPieces();
	}
	{
		auto lock = CModelsLock::GetScopedLock(); // working with S3DModelVAO needs locking
		auto& inst = S3DModelVAO::GetInstance();
		inst.ProcessVertices(model);
		inst.ProcessIndicies(model);
		model->loadStatus = S3DModel::LoadStatus::LOADED;
	}
	cv.notify_all();
}

void CModelLoader::Upload(S3DModel* model) const {
	RECOIL_DETAILED_TRACY_ZONE;
	if (model->uploaded) //already uploaded
		return;

	assert(Threading::IsMainThread() || Threading::IsGameLoadThread());

	{
		auto lock = CLoadLock::GetUniqueLock(); //mostly needed to support calls from CFeatureHandler::LoadFeaturesFromMap()
		S3DModelVAO::GetInstance().UploadVBOs();

		// 3DO atlases are preloaded C3DOTextureHandler::Init()
		if (model->type != MODELTYPE_3DO) {
			// make sure textures (already preloaded) are fully loaded
			textureHandlerS3O.LoadTexture(model);
		}
	}

	for (auto* p : model->pieceObjects) {
		p->ReleaseShatterIndices();
	}

	// warn about models with bad normals (they break lighting)
	// skip for 3DO's since those have auto-calculated normals
	if (model->type != MODELTYPE_3DO)
		CheckPieceNormals(model, model->GetRootPiece());

	model->uploaded = true;
}

