/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <algorithm>
#include <array>
#include <cstdio>
#include <memory>
#include <semaphore>
#include <random>
#include <chrono>

#include <sys/types.h>
#include <sys/stat.h>

#include <fmt/format.h>

#include "ArchiveNameResolver.h"
#include "ArchiveScanner.h"
#include "ArchiveLoader.h"
#include "DataDirLocater.h"
#include "Archives/IArchive.h"
#include "Archives/DirArchive.h"
#include "FileFilter.h"
#include "DataDirsAccess.h"
#include "FileSystem.h"
#include "FileQueryFlags.h"
#include "Lua/LuaParser.h"
#include "System/ContainerUtil.h"
#include "System/StringUtil.h"
#include "System/Exceptions.h"
#include "System/Threading/ThreadPool.h"
#include "System/FileSystem/RapidHandler.h"
#include "System/FileSystem/Archives/PoolArchive.h"
#include "System/Log/ILog.h"
#include "System/Threading/SpringThreading.h"
#include "System/UnorderedMap.hpp"
#include "System/UnorderedSet.hpp"

#if !defined(DEDICATED) && !defined(UNITSYNC)
	#include "System/TimeProfiler.h"
	#include "System/Platform/Watchdog.h"
#endif


#define LOG_SECTION_ARCHIVESCANNER "ArchiveScanner"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_ARCHIVESCANNER)

#define ACRHIVE_CHECKSUM_DUMP 1

/*
 * The archive scanner is used to find stuff in archives
 * which are needed before building the virtual filesystem.
 * This currently includes maps and mods.
 * It uses caching to speed up the process.
 *
 * It only retrieves info that is used in an initial listing.
 * For detailed info when selecting a map for example,
 * the more specialized parsers will be used.
 * (mapping one archive when selecting a map is not slow,
 * but mapping them all, every time to make the list is)
 */

constexpr static int INTERNAL_VER = 20;


/*
 * Engine known (and used?) tags in [map|mod]info.lua
 */
struct KnownInfoTag {
	std::string name;
	std::string desc;
	bool required;
};

const std::array<KnownInfoTag, 12> knownTags = {
	KnownInfoTag{"name",        "example: Original Total Annihilation",                            true},
	KnownInfoTag{"shortname",   "example: OTA",                                                   false},
	KnownInfoTag{"version",     "example: v2.3",                                                  false},
	KnownInfoTag{"mutator",     "example: deployment",                                            false},
	KnownInfoTag{"game",        "example: Total Annihilation",                                    false},
	KnownInfoTag{"shortgame",   "example: TA",                                                    false},
	KnownInfoTag{"description", "example: Little units blowing up other little units",            false},
	KnownInfoTag{"mapfile",     "in case its a map, store location of smf file",                  false}, //FIXME is this ever used in the engine?! or does it auto calc the location?
	KnownInfoTag{"modtype",     "0=hidden, 1=primary, (2=unused), 3=map, 4=base, 5=menu",          true},
	KnownInfoTag{"depend",      "a table with all archives that needs to be loaded for this one", false},
	KnownInfoTag{"replace",     "a table with archives that got replaced with this one",          false},
	KnownInfoTag{"onlyLocal",   "if true spring will not listen for incoming connections",        false}
};

const spring::unordered_map<std::string, bool> baseContentArchives = {
	{      "bitmaps.sdz", true},
	{"springcontent.sdz", true},
	{    "maphelper.sdz", true},
	{      "cursors.sdz", true},
};

// 1: commonly read from all archives when scanning through them
// 2: less commonly used, or only when looking at a specific archive
//    (for example when hosting Game-X)
//
// Lobbies get the unit list from unitsync. Unitsync gets it by executing
// gamedata/defs.lua, which loads units, features, weapons, movetypes and
// armors (that is why armor.txt is in the list).
const spring::unordered_map<std::string, int> metaFileClasses = {
	{      "mapinfo.lua", 1},   // basic archive info
	{      "modinfo.lua", 1},   // basic archive info
	{   "modoptions.lua", 2},   // used by lobbies
	{"engineoptions.lua", 2},   // used by lobbies
	{    "validmaps.lua", 2},   // used by lobbies
	{        "luaai.lua", 2},   // used by lobbies
	{        "armor.txt", 2},   // used by lobbies (disabled units list)
	{ "springignore.txt", 2},   // used by lobbies (disabled units list)
};

const spring::unordered_map<std::string, int> metaDirClasses = {
	{"sidepics/", 2},   // used by lobbies
	{"gamedata/", 2},   // used by lobbies
	{   "units/", 2},   // used by lobbies (disabled units list)
	{"features/", 2},   // used by lobbies (disabled units list)
	{ "weapons/", 2},   // used by lobbies (disabled units list)
};


CArchiveScanner* archiveScanner = nullptr;


/*
 * CArchiveScanner::ArchiveData
 */
CArchiveScanner::ArchiveData::ArchiveData(const LuaTable& archiveTable, bool fromCache)
{
	if (!archiveTable.IsValid())
		return;

	std::vector<std::string> keys;
	if (!archiveTable.GetKeys(keys))
		return;

	for (std::string& key: keys) {
		const std::string& keyLower = StringToLower(key);

		if (ArchiveData::IsReservedKey(keyLower))
			continue;

		if (keyLower == "modtype") {
			SetInfoItemValueInteger(key, archiveTable.GetInt(key, 0));
			continue;
		}

		switch (archiveTable.GetType(key)) {
			case LuaTable::STRING: {
				SetInfoItemValueString(key, archiveTable.GetString(key, ""));
			} break;
			case LuaTable::NUMBER: {
				SetInfoItemValueFloat(key, archiveTable.GetFloat(key, 0.0f));
			} break;
			case LuaTable::BOOLEAN: {
				SetInfoItemValueBool(key, archiveTable.GetBool(key, false));
			} break;
			default: {
				// just ignore unsupported types (most likely to be lua-tables)
				//throw content_error("Lua-type " + IntToString(luaType) + " not supported in archive-info, but it is used on key \"" + *key + "\"");
			} break;
		}
	}

	const LuaTable& _dependencies = archiveTable.SubTable("depend");
	const LuaTable& _replaces = archiveTable.SubTable("replace");

	for (int dep = 1; _dependencies.KeyExists(dep); ++dep) {
		dependencies.push_back(_dependencies.GetString(dep, ""));
	}
	for (int rep = 1; _replaces.KeyExists(rep); ++rep) {
		replaces.push_back(_replaces.GetString(rep, ""));
	}


	// FIXME
	// XXX HACK needed until lobbies, lobbyserver and unitsync are sorted out
	// so they can uniquely identify different versions of the same mod.
	// (at time of this writing they use name only)
	//
	// NOTE when changing this, this function is used both by the code that
	// reads ArchiveCache.lua and the code that reads modinfo.lua from the mod.
	// so make sure it doesn't keep adding stuff to the name everytime
	// Spring/unitsync is loaded.
	//
	const std::string& name = GetNameVersioned();
	const std::string& version = GetVersion();

	if (!version.empty()) {
		if (name.find(version) == std::string::npos) {
			SetInfoItemValueString("name", name + " " + version);
		} else if (!fromCache) {
			LOG_L(L_WARNING, "[%s] version \"%s\" included in name \"%s\"", __func__, version.c_str(), name.c_str());
		}
	}


	if (GetName().empty())
		SetInfoItemValueString("name_pure", name);
}

std::string CArchiveScanner::ArchiveData::GetKeyDescription(const std::string& keyLower)
{
	const auto pred = [&keyLower](const KnownInfoTag& t) { return (t.name == keyLower); };
	const auto iter = std::find_if(knownTags.cbegin(), knownTags.cend(), pred);

	if (iter != knownTags.cend())
		return (iter->desc);

	return "<custom property>";
}


bool CArchiveScanner::ArchiveData::IsReservedKey(const std::string& keyLower)
{
	return ((keyLower == "depend") || (keyLower == "replace"));
}


bool CArchiveScanner::ArchiveData::IsValid(std::string& err) const
{
	using P = decltype(infoItems)::value_type;

	const auto HasInfoItem = [this](const std::string& name) {
		const auto pred = [](const P& a, const P& b) { return (a.first < b.first); };
		const auto iter = std::lower_bound(infoItems.begin(), infoItems.end(), decltype(infoItems)::value_type{name, {}}, pred);
		return (iter != infoItems.end() && iter->first == name);
	};

	// look for any info-item that is required but not present
	// datums containing only optional items ("shortgame", etc)
	// are allowed
	const auto pred = [&](const KnownInfoTag& tag) { return (tag.required && !HasInfoItem(tag.name)); };
	const auto iter = std::find_if(knownTags.cbegin(), knownTags.cend(), pred);

	if (iter == knownTags.cend())
		return true;

	err = "Missing required tag \"" + iter->name + "\".";
	return false;
}



const InfoItem* CArchiveScanner::ArchiveData::GetInfoItem(const std::string& key) const
{
	using P = decltype(infoItems)::value_type;

	const std::string& lcKey = StringToLower(key);

	const auto pred = [](const P& a, const P& b) { return (a.first < b.first); };
	const auto iter = std::lower_bound(infoItems.begin(), infoItems.end(), P{lcKey, {}}, pred);

	if (iter != infoItems.end() && iter->first == lcKey)
		return &(iter->second);

	return nullptr;
}



InfoItem& CArchiveScanner::ArchiveData::GetAddInfoItem(const std::string& key)
{
	const std::string& keyLower = StringToLower(key);

	if (IsReservedKey(keyLower))
		throw content_error("You may not use key " + key + " in archive info, as it is reserved.");

	using P = decltype(infoItems)::value_type;

	const auto pred = [](const P& a, const P& b) { return (a.first < b.first); };
	const auto iter = std::lower_bound(infoItems.begin(), infoItems.end(), P{keyLower, {}}, pred);

	if (iter == infoItems.end() || iter->first != keyLower) {
		// add a new info-item, sorted by lower-case key
		InfoItem infoItem;
		infoItem.key = key;
		infoItem.valueType = INFO_VALUE_TYPE_INTEGER;
		infoItem.value.typeInteger = 0;
		infoItems.emplace_back(keyLower, std::move(infoItem));

		// swap into position
		for (size_t i = infoItems.size() - 1; i > 0; i--) {
			if (infoItems[i - 1].first < infoItems[i].first)
				return (infoItems[i].second);

			std::swap(infoItems[i - 1], infoItems[i]);
		}

		return (infoItems[0].second);
	}

	return (iter->second);
}

void CArchiveScanner::ArchiveData::SetInfoItemValueString(const std::string& key, const std::string& value)
{
	InfoItem& infoItem = GetAddInfoItem(key);
	infoItem.valueType = INFO_VALUE_TYPE_STRING;
	infoItem.valueTypeString = value;
}

void CArchiveScanner::ArchiveData::SetInfoItemValueInteger(const std::string& key, int value)
{
	InfoItem& infoItem = GetAddInfoItem(key);
	infoItem.valueType = INFO_VALUE_TYPE_INTEGER;
	infoItem.value.typeInteger = value;
}

void CArchiveScanner::ArchiveData::SetInfoItemValueFloat(const std::string& key, float value)
{
	InfoItem& infoItem = GetAddInfoItem(key);
	infoItem.valueType = INFO_VALUE_TYPE_FLOAT;
	infoItem.value.typeFloat = value;
}

void CArchiveScanner::ArchiveData::SetInfoItemValueBool(const std::string& key, bool value)
{
	InfoItem& infoItem = GetAddInfoItem(key);
	infoItem.valueType = INFO_VALUE_TYPE_BOOL;
	infoItem.value.typeBool = value;
}


std::vector<InfoItem> CArchiveScanner::ArchiveData::GetInfoItems() const
{
	std::vector<InfoItem> retInfoItems;

	retInfoItems.reserve(infoItems.size());

	for (const auto& pair: infoItems) {
		retInfoItems.push_back(pair.second);
		retInfoItems.back().desc = GetKeyDescription(pair.first);
	}

	return retInfoItems;
}


std::string CArchiveScanner::ArchiveData::GetInfoValueString(const std::string& key) const
{
	const InfoItem* infoItem = GetInfoItem(key);

	if (infoItem != nullptr) {
		if (infoItem->valueType == INFO_VALUE_TYPE_STRING)
			return infoItem->valueTypeString;

		return (infoItem->GetValueAsString());
	}

	return "";
}

int CArchiveScanner::ArchiveData::GetInfoValueInteger(const std::string& key) const
{
	const InfoItem* infoItem = GetInfoItem(key);

	if ((infoItem != nullptr) && (infoItem->valueType == INFO_VALUE_TYPE_INTEGER))
		return (infoItem->value.typeInteger);

	return 0;
}

float CArchiveScanner::ArchiveData::GetInfoValueFloat(const std::string& key) const
{
	const InfoItem* infoItem = GetInfoItem(key);

	if ((infoItem != nullptr) && (infoItem->valueType == INFO_VALUE_TYPE_FLOAT))
		return (infoItem->value.typeFloat);

	return 0.0f;
}

bool CArchiveScanner::ArchiveData::GetInfoValueBool(const std::string& key) const
{
	const InfoItem* infoItem = GetInfoItem(key);

	if ((infoItem != nullptr) && (infoItem->valueType == INFO_VALUE_TYPE_BOOL))
		return (infoItem->value.typeBool);

	return false;
}



static spring::recursive_mutex scannerMutex;
static std::atomic<uint32_t> numScannedArchives{0};


/*
 * CArchiveScanner
 */

CArchiveScanner::CArchiveScanner()
{
	scanArchiveMutex.SetThreadSafety(false);
	ReadCache();
}


CArchiveScanner::~CArchiveScanner()
{
	WriteCache();
}

uint32_t CArchiveScanner::GetNumScannedArchives()
{
	// needs to be a static since archiveScanner remains null until ctor returns
	return (numScannedArchives.load());
}


void CArchiveScanner::Clear()
{
	archiveInfos.clear();
	archiveInfos.reserve(256);
	archiveInfosIndex.clear();
	archiveInfosIndex.reserve(256);
	brokenArchives.clear();
	brokenArchives.reserve(16);
	poolFilesInfo.clear();
	poolFilesInfo.reserve(32768); //be generous
	brokenArchivesIndex.clear();
	brokenArchivesIndex.reserve(16);
	cacheFile.clear();
	numFilesHashed.store(0);
}

void CArchiveScanner::Reload()
{
	// {Read,Write,Scan}* all grab this too but we need the entire reloading-sequence to appear atomic
	std::lock_guard<decltype(scannerMutex)> lck(scannerMutex);

	// dtor
	WriteCache();

	// ctor
	ReadCache();
}

void CArchiveScanner::ScanAllDirs()
{
	std::lock_guard<decltype(scannerMutex)> lck(scannerMutex);

	const std::vector<std::string>& dataDirPaths = dataDirLocater.GetDataDirPaths();
	const std::vector<std::string>& dataDirRoots = dataDirLocater.GetDataDirRoots();

	std::vector<std::string> scanDirs;
	scanDirs.reserve(dataDirPaths.size() * dataDirRoots.size());

	// last specified is first scanned
	for (auto d = dataDirPaths.rbegin(); d != dataDirPaths.rend(); ++d) {
		for (const std::string& s: dataDirRoots) {
			if (s.empty())
				continue;

			scanDirs.emplace_back(*d + s);
		}
	}

	// ArchiveCache has been parsed at this point --> archiveInfos is populated
#if !defined(DEDICATED) && !defined(UNITSYNC)
	SCOPED_ONCE_TIMER("CArchiveScanner::ScanAllDirs");
#endif

	ScanDirs(scanDirs);
	WriteCacheData(GetFilepath());
}


void CArchiveScanner::ScanDirs(const std::vector<std::string>& scanDirs)
{
	std::lock_guard<decltype(scannerMutex)> lck(scannerMutex);
	std::deque<std::string> foundArchives;

	isDirty = true;

	scanArchiveMutex.SetThreadSafety(true);
	// scan for all archives
	for (const std::string& dir: scanDirs) {
		if (!FileSystem::DirExists(dir))
			continue;

		LOG("Scanning: %s", dir.c_str());
		ScanDir(dir, foundArchives);
		ScanArchives(dir, foundArchives);
		foundArchives.clear();
	}
	scanArchiveMutex.SetThreadSafety(false);

	// Now we'll have to parse the replaces-stuff found in the mods
	for (const auto& archiveInfo: archiveInfos) {
		const std::string& lcOriginalName = StringToLower(archiveInfo.origName);

		for (const std::string& replaceName: archiveInfo.archiveData.GetReplaces()) {
			const std::string& lcReplaceName = StringToLower(replaceName);

			// Overwrite the info for this archive with a replaced pointer
			ArchiveInfo& ai = GetAddArchiveInfo(lcReplaceName);

			ai.path = "";
			ai.origName = replaceName;
			ai.modified = 1;
			ai.archiveData = {};
			ai.updated = true;
			ai.replaced = lcOriginalName;
		}
	}
}


void CArchiveScanner::ScanDir(const std::string& curPath, std::deque<std::string>& foundArchives)
{
	std::deque<std::string> subDirs = {curPath};

	while (!subDirs.empty()) {
		#if !defined(DEDICATED) && !defined(UNITSYNC)
			Watchdog::ClearTimer();
		#endif

		const std::string& subDir = FileSystem::EnsurePathSepAtEnd(subDirs.front());
		const std::vector<std::string>& foundFiles = dataDirsAccess.FindFiles(subDir, "*", FileQueryFlags::INCLUDE_DIRS);

		subDirs.pop_front();

		for (const std::string& fileName: foundFiles) {
			const std::string& fileNameNoSep = FileSystem::EnsureNoPathSepAtEnd(fileName);
			const std::string& lcFilePath = StringToLower(FileSystem::GetDirectory(fileNameNoSep));

			// Exclude archive files found inside directory archives (.sdd)
			if (lcFilePath.find(".sdd") != std::string::npos)
				continue;

			// Is this an archive we should look into?
			if (archiveLoader.IsArchiveFile(fileNameNoSep)) {
				foundArchives.push_front(fileNameNoSep); // push in reverse order!
				continue;
			}
			if (FileSystem::DirExists(fileNameNoSep)) {
				subDirs.push_back(fileNameNoSep);
			}
		}
	}
}

void CArchiveScanner::ScanArchives(const std::string& curPath, const std::deque<std::string>& foundArchives)
{
#ifdef _WIN32
	static constexpr int NUM_PARALLEL_FILE_READS_SD = 4;
#else
	// Linux FS even on spinning disk seems far more tolerant to parallel reads, use all threads
	const int NUM_PARALLEL_FILE_READS_SD = ThreadPool::GetNumThreads();
#endif // _WIN32

	const auto isOnSpinningDisk = FileSystem::IsPathOnSpinningDisk(curPath);
	int numParallelFileReads = isOnSpinningDisk ? NUM_PARALLEL_FILE_READS_SD : ThreadPool::GetNumThreads();

	numParallelFileReads = std::min(numParallelFileReads, ThreadPool::GetNumThreads());
	// limit the number of simultaneous IO operations
	std::counting_semaphore sem(numParallelFileReads);

	// Create archiveInfos etc. if not in cache already
	for_mt(0, foundArchives.size(), [&foundArchives, &sem, this](int fai) {
		const std::string& archive = foundArchives[fai];

		sem.acquire();
		ScanArchive(archive, false);
		sem.release();

#if !defined(DEDICATED) && !defined(UNITSYNC)
		Watchdog::ClearTimer(WDT_VFSI);
#endif
	});
}

static void AddDependency(std::vector<std::string>& deps, const std::string& dependency)
{
	spring::VectorInsertUnique(deps, dependency, true);
}

bool CArchiveScanner::CheckCompression(const IArchive* ar, const std::string& fullName, std::string& error)
{
	if (!ar->CheckForSolid())
		return true;

	for (uint32_t fid = 0; fid != ar->NumFiles(); ++fid) {
		if (ar->HasLowReadingCost(fid))
			continue;

		const auto& fn = ar->FileName(fid);

		switch (GetMetaFileClass(StringToLower(fn))) {
			case 1: {
				error += "reading primary meta-file " + fn + " too expensive; ";
				error += "please repack this archive with non-solid compression";
				return false;
			} break;
			case 2: {
				LOG_SL(LOG_SECTION_ARCHIVESCANNER, L_WARNING, "Archive %s: reading secondary meta-file %s too expensive", fullName.c_str(), fn.c_str());
			} break;
			case 0:
			default: {
				continue;
			} break;
		}
	}

	return true;
}

std::string CArchiveScanner::SearchMapFile(const IArchive* ar, std::string& error)
{
	assert(ar != nullptr);

	// check for smf and if the uncompression of important files is too costy
	for (uint32_t fid = 0; fid != ar->NumFiles(); ++fid) {
		const auto& fn = ar->FileName(fid);
		const std::string ext = FileSystem::GetExtension(StringToLower(fn));

		if (ext == "smf")
			return fn;
	}

	return "";
}


void CArchiveScanner::ReadCache()
{
	Clear();

    cacheFile = FileSystem::EnsurePathSepAtEnd(FileSystem::GetCacheDir()) + IntToString(INTERNAL_VER, "ArchiveCache%i.lua");

	if (!FileSystem::FileExists(cacheFile)) {
		// Try to save initial scanning of assets, but will have to redo hashing
		// as the previous version had bugs in that area
		// probe two previous versions
		std::array prevCacheFiles {
			FileSystem::EnsurePathSepAtEnd(FileSystem::GetCacheDir()) + IntToString(INTERNAL_VER - 1, "ArchiveCache%i.lua"),
			FileSystem::EnsurePathSepAtEnd(FileSystem::GetCacheDir()) + IntToString(INTERNAL_VER - 2, "ArchiveCache%i.lua"),
			FileSystem::EnsurePathSepAtEnd(FileSystem::GetCacheDir()) + IntToString(INTERNAL_VER - 3, "ArchiveCache%i.lua")
		};

		for (const auto& prevCacheFile : prevCacheFiles) {
			if (!ReadCacheData(prevCacheFile, true))
				continue;

			// nullify hashes, filesInfo
			for (auto& ai : archiveInfos) {
				ai.checksum = sha512::NULL_RAW_DIGEST;
				ai.hashed = false;
				ai.filesInfo.clear();
			}

			// Also nullify pool info
			poolFilesInfo.clear();
			isDirty = true;

			break; // on first success
		}
	}

	ReadCacheData(GetFilepath());
	ScanAllDirs();
}

void CArchiveScanner::WriteCache()
{
	if (!isDirty)
		return;

	WriteCacheData(GetFilepath());
}

CArchiveScanner::ArchiveInfo& CArchiveScanner::GetAddArchiveInfo(const std::string& lcfn)
{
	auto aiIter = archiveInfosIndex.find(lcfn);
	auto aiPair = std::make_pair(aiIter, 0);

	if (aiIter == archiveInfosIndex.end()) {
		aiPair = archiveInfosIndex.emplace(lcfn, archiveInfos.size());
		aiIter = aiPair.first;
		archiveInfos.emplace_back();
	}

	return archiveInfos[aiIter->second];
}

CArchiveScanner::BrokenArchive& CArchiveScanner::GetAddBrokenArchive(const std::string& lcfn)
{
	auto baIter = brokenArchivesIndex.find(lcfn);
	auto baPair = std::make_pair(baIter, false);

	if (baIter == brokenArchivesIndex.end()) {
		baPair = brokenArchivesIndex.emplace(lcfn, brokenArchives.size());
		baIter = baPair.first;
		brokenArchives.emplace_back();
	}

	return brokenArchives[baIter->second];
}


void CArchiveScanner::ScanArchive(const std::string& fullName, bool doChecksum)
{
	uint32_t modifiedTime = 0;

	if (CheckCachedData(fullName, modifiedTime, doChecksum))
		return;

	isDirty = true;

	const std::string& fname = FileSystem::GetFilename(fullName);
	const std::string& fpath = FileSystem::GetDirectory(fullName);
	const std::string& lcfn  = StringToLower(fname);

	std::unique_ptr<IArchive> ar(archiveLoader.OpenArchive(fullName));

	if (ar == nullptr || !ar->IsOpen()) {
		auto lck = scanArchiveMutex.GetScopedLock();

		LOG_L(L_WARNING, "[AS::%s] unable to open archive \"%s\"", __func__, fullName.c_str());

		// record it as broken, so we don't need to look inside everytime
		BrokenArchive& ba = GetAddBrokenArchive(lcfn);
		ba.name = lcfn;
		ba.path = fpath;
		ba.modified = modifiedTime;
		ba.updated = true;
		ba.problem = "Unable to open archive";

		// does not count as a scan
		// numScannedArchives += 1;
		return;
	}

	std::string error;
	std::string arMapFile; // file in archive with "smf" extension
	std::string miMapFile; // value for the 'mapfile' key parsed from mapinfo
	std::string luaInfoFile;

	const bool hasModInfo = ar->FileExists("modinfo.lua");
	const bool hasMapInfo = ar->FileExists("mapinfo.lua");


	ArchiveInfo ai;
	ArchiveData& ad = ai.archiveData;

	// execute the respective .lua, otherwise assume this archive is a map
	if (hasMapInfo) {
		ScanArchiveLua(ar.get(), luaInfoFile = "mapinfo.lua", ai, error);

		if ((miMapFile = ad.GetMapFile()).empty()) {
			if (ar->GetType() != ARCHIVE_TYPE_SDV)
				LOG_L(L_WARNING, "[AS::%s] set the 'mapfile' key in mapinfo.lua of archive \"%s\" for faster loading!", __func__, fullName.c_str());

			arMapFile = SearchMapFile(ar.get(), error);
		}
	} else if (hasModInfo) {
		ScanArchiveLua(ar.get(), luaInfoFile = "modinfo.lua", ai, error);
	} else {
		arMapFile = SearchMapFile(ar.get(), error);
	}

	if (!CheckCompression(ar.get(), fullName, error)) {
		auto lck = scanArchiveMutex.GetScopedLock();

		LOG_L(L_WARNING, "[AS::%s] failed to scan \"%s\" (%s)", __func__, fullName.c_str(), error.c_str());

		// mark archive as broken, so we don't need to look inside everytime
		BrokenArchive& ba = GetAddBrokenArchive(lcfn);
		ba.name = lcfn;
		ba.path = fpath;
		ba.modified = modifiedTime;
		ba.updated = true;
		ba.problem = error;

		// does count as a scan
		numScannedArchives += 1;
		return;
	}

	if (hasMapInfo || !arMapFile.empty()) {
		auto lck = scanArchiveMutex.GetScopedLock();
		// map archive
		// FIXME: name will never be empty if version is set (see HACK in ArchiveData)
		if ((ad.GetName()).empty()) {
			ad.SetInfoItemValueString("name_pure", FileSystem::GetBasename(arMapFile));
			ad.SetInfoItemValueString("name", FileSystem::GetBasename(arMapFile));
		}

		if (miMapFile.empty())
			ad.SetInfoItemValueString("mapfile", arMapFile);

		AddDependency(ad.GetDependencies(), GetMapHelperContentName());
		ad.SetInfoItemValueInteger("modType", modtype::map);

		LOG_S(LOG_SECTION_ARCHIVESCANNER, "Found new map: %s", ad.GetNameVersioned().c_str());
	} else if (hasModInfo) {
		auto lck = scanArchiveMutex.GetScopedLock();
		// game or base-type (cursors, bitmaps, ...) archive
		// babysitting like this is really no longer required
		if (ad.IsGame() || ad.IsMenu())
			AddDependency(ad.GetDependencies(), GetSpringBaseContentName());

		LOG_S(LOG_SECTION_ARCHIVESCANNER, "Found new game: %s", ad.GetNameVersioned().c_str());
	} else {
		// neither a map nor a mod: error
		LOG_S(LOG_SECTION_ARCHIVESCANNER, "missing modinfo.lua/mapinfo.lua");
	}

	ai.path = fpath;
	ai.modified = modifiedTime;

	// Store modinfo.lua/mapinfo.lua modified timestamp for directory archives, as only they can change.
	if (ar->GetType() == ARCHIVE_TYPE_SDD && !luaInfoFile.empty()) {
		ai.archiveDataPath = ar->GetArchiveFile() + "/" + static_cast<const CDirArchive*>(ar.get())->FileName(ar->FindFile(luaInfoFile));
		ai.modifiedArchiveData = FileSystemAbstraction::GetFileModificationTime(ai.archiveDataPath);
	}

	ai.origName = fname;
	ai.updated = true;
	ai.hashed = doChecksum && GetArchiveChecksum(fullName, ai);

	{
		auto lck = scanArchiveMutex.GetScopedLock();
		archiveInfosIndex.emplace(lcfn, archiveInfos.size());
		archiveInfos.emplace_back(std::move(ai));
	}

	numScannedArchives += 1;
}


bool CArchiveScanner::CheckCachedData(const std::string& fullName, uint32_t& modified, bool doChecksum)
{
	// virtual archives do not exist on disk, and thus do not have a modification time
	// they should still be scanned as normal archives so we only skip the cache-check
	if (FileSystem::GetExtension(fullName) == "sva")
		return false;

	// if stat fails, assume the archive is not broken nor cached
	// it would also fail in the case of virtual archives and cause
	// warning-spam which is suppressed by the extension-test above
	if ((modified = FileSystemAbstraction::GetFileModificationTime(fullName)) == 0)
		return false;

	const std::string& fileName = FileSystem::GetFilename(fullName);
	const std::string& filePath = FileSystem::GetDirectory(fullName);
	const std::string& fileNameLower = StringToLower(fileName);


	const auto baIter = brokenArchivesIndex.find(fileNameLower);
	const auto aiIter = archiveInfosIndex.find(fileNameLower);

	// determine whether this archive has earlier be found to be broken
	if (baIter != brokenArchivesIndex.end()) {
		BrokenArchive& ba = brokenArchives[baIter->second];

		if (modified == ba.modified && filePath == ba.path)
			return (ba.updated = true);
	}


	// determine whether to rely on the cached info or not
	if (aiIter == archiveInfosIndex.end())
		return false;

	const size_t archiveIndex = aiIter->second;

	ArchiveInfo& ai = archiveInfos[archiveIndex];
	ArchiveInfo& rai = archiveInfos[archiveInfos.size() - 1];

	// this archive may have been obsoleted, do not process it if so
	if (!ai.replaced.empty())
		return true;

	const bool haveValidCacheData = (modified == ai.modified && filePath == ai.path);
	// check if the archive data file (modinfo.lua/mapinfo.lua) has changed
	const bool archiveDataChanged = (!ai.archiveDataPath.empty() && FileSystemAbstraction::GetFileModificationTime(ai.archiveDataPath) != ai.modifiedArchiveData);

	if (haveValidCacheData && !archiveDataChanged) {
		// archive found in cache, update checksum if wanted
		// this also has to flag isDirty or ArchiveCache will
		// not be rewritten even if the hash silently changed,
		// e.g. after redownload
		ai.updated = true;

		if (doChecksum && !ai.hashed) {
			isDirty |= (ai.hashed = GetArchiveChecksum(fullName, ai));
		}
		assert(!doChecksum || ai.checksum != sha512::NULL_RAW_DIGEST);

		return true;
	}

	if (ai.updated) {
		LOG_L(L_ERROR, "[AS::%s] found a \"%s\" already in \"%s\", ignoring.", __func__, fullName.c_str(), (ai.path + ai.origName).c_str());

		if (baseContentArchives.find(aiIter->first) == baseContentArchives.end())
			return true; // ignore

		throw user_error(
			std::string("duplicate base content detected:\n\t") + ai.path +
			std::string("\n\t") + filePath +
			std::string("\nPlease fix your configuration/installation as this can cause desyncs!")
		);
	}

	{
		auto lck = scanArchiveMutex.GetScopedLock();

		// if we are here, we could have invalid info in the cache
		// force a reread if it is a directory archive (.sdd), as
		// st_mtime only reflects changes to the directory itself
		// (not the contents)
		//
		// remap replacement archive
		if (archiveIndex != (archiveInfos.size() - 1)) {
			archiveInfosIndex[StringToLower(rai.origName)] = archiveIndex;
			archiveInfos[archiveIndex] = std::move(rai);
		}

		archiveInfosIndex.erase(fileNameLower);
		archiveInfos.pop_back();
		return false;
	}
}


bool CArchiveScanner::ScanArchiveLua(IArchive* ar, const std::string& fileName, ArchiveInfo& ai, std::string& err)
{
	std::vector<std::uint8_t> buf;

	if (!ar->GetFile(fileName, buf) || buf.empty()) {
		err = "Error reading " + fileName;

		if (ar->GetArchiveFile().find(".sdp") != std::string::npos)
			err += " (archive's rapid tag: " + GetRapidTagFromPackage(FileSystem::GetBasename(ar->GetArchiveFile())) + ")";

		return false;
	}

	// NB: skips LuaConstGame::PushEntries(L) since that would invoke ScanArchive again
	LuaParser p(std::string((char*)(buf.data()), buf.size()), SPRING_VFS_ZIP);

	if (!p.Execute()) {
		err = "Error in " + fileName + ": " + p.GetErrorLog();
		return false;
	}

	try {
		ai.archiveData = CArchiveScanner::ArchiveData(p.GetRoot(), false);

		if (!ai.archiveData.IsValid(err)) {
			err = "Error in " + fileName + ": " + err;
			return false;
		}
	} catch (const content_error& contErr) {
		err = "Error in " + fileName + ": " + contErr.what();
		return false;
	}

	return true;
}

IFileFilter* CArchiveScanner::CreateIgnoreFilter(IArchive* ar)
{
	IFileFilter* ignore = IFileFilter::Create();
	std::vector<std::uint8_t> buf;

	ignore->AddRuleRegex("^\\..*$");

	// this automatically splits lines
	if (ar->GetFile("springignore.txt", buf) && !buf.empty())
		ignore->AddRule(std::string((char*)(&buf[0]), buf.size()));

	return ignore;
}



/**
 * Get checksum of the data in the specified archive.
 * Returns 0 if file could not be opened.
 */
bool CArchiveScanner::GetArchiveChecksum(const std::string& archiveName, ArchiveInfo& archiveInfo)
{
	// try to open an archive
	std::unique_ptr<IArchive> ar(archiveLoader.OpenArchive(archiveName));

	if (ar == nullptr)
		return false;

#ifdef _WIN32
	static constexpr int NUM_PARALLEL_FILE_READS_SD = 4;
#else
	// Linux FS even on spinning disk seems far more tolerant to parallel reads, use all threads
	const int NUM_PARALLEL_FILE_READS_SD = ThreadPool::GetNumThreads();
#endif // _WIN32

	int numParallelFileReads;

	switch (ar->GetType())
	{
		case ARCHIVE_TYPE_SDP: {
			auto isOnSpinningDisk = FileSystem::IsPathOnSpinningDisk(CPoolArchive::GetPoolRootDirectory(archiveName));
			// each file is one gzip instance, can MT
			numParallelFileReads = isOnSpinningDisk ? NUM_PARALLEL_FILE_READS_SD : ThreadPool::GetNumThreads();
		} break;
		case ARCHIVE_TYPE_SDD: {
			auto isOnSpinningDisk = FileSystem::IsPathOnSpinningDisk(archiveName);
			// just a file, can MT
			numParallelFileReads = isOnSpinningDisk ? NUM_PARALLEL_FILE_READS_SD : ThreadPool::GetNumThreads();
		} break;
		case ARCHIVE_TYPE_SDZ: {
			numParallelFileReads = ThreadPool::GetNumThreads(); // will open NumThreads parallel archives, this way GetFile() is no longer needs to be mutex locked
		} break;
		case ARCHIVE_TYPE_SD7: {
			numParallelFileReads = !ar->CheckForSolid() ? ThreadPool::GetNumThreads() : 1; // allow parallel access, but only for non-solid archives
		} break;
		default: { // just default to 1 thread
			numParallelFileReads = 1;
		} break;
	}

	numParallelFileReads = std::min(numParallelFileReads, ThreadPool::GetNumThreads());

	const bool sdpArchive = (ar->GetType() == ARCHIVE_TYPE_SDP);
	const bool compressedArchive = (ar->GetType() == ARCHIVE_TYPE_SD7 || ar->GetType() == ARCHIVE_TYPE_SDZ);

	// limit the number of simultaneous IO operations
	std::counting_semaphore sem(numParallelFileReads);

	// load ignore list
	std::unique_ptr<IFileFilter> ignore(CreateIgnoreFilter(ar.get()));

	// warm up. For some archive types ar->FileInfo(fid) is a mutable operation loading important IArchive::SFileInfo fields
	std::atomic_uint32_t numFiles = {0};
	for_mt(0, ar->NumFiles(), [&sem, &numFiles, &ar = std::as_const(ar), &ignore = std::as_const(ignore)](int fid) {
		const auto fn = ar->FileName(fid);

		if (ignore->Match(fn))
			return;

		sem.acquire();
		const auto volatile fi = ar->FileInfo(fid); // volatile to force execution
		sem.release();
		++numFiles;
	});

	// store relevant lowercased filenames from the archive
	std::vector<std::string> fileNames;

	fileNames.reserve(numFiles.load());
	archiveInfo.filesInfo.reserve(numFiles.load());

	for (uint32_t fid = 0; fid < ar->NumFiles(); ++fid) {
		auto fi = ar->FileInfo(fid);

		if (ignore->Match(fi.fileName))
			continue;

		// special treatment of SDP archives: insert information from poolFilesInfo
		if (sdpArchive) {
			auto it = poolFilesInfo.find(fi.specialFileName); // fi.specialFileName contains pool file name (prefix/suffix.gz)
			if (it != poolFilesInfo.end()) {
				archiveInfo.filesInfo[fi.fileName] = it->second;
			}
		}

		auto it = archiveInfo.filesInfo.find(fi.fileName);
		if (it == archiveInfo.filesInfo.end())
			it = archiveInfo.filesInfo.emplace(fi.fileName, {}).first;

		if (fi.modTime != it->second.modTime || fi.size != it->second.size) {
			it->second.modTime = fi.modTime;
			it->second.size = fi.size;
			it->second.checksum = sha512::NULL_RAW_DIGEST;
		}

		fileNames.emplace_back(std::move(fi.fileName));
	}

	std::array<std::vector<uint8_t>, ThreadPool::MAX_THREADS> fileBuffers;

	for_mt(0, fileNames.size(), [&ar, &fileNames = std::as_const(fileNames), &fileBuffers, &filesInfo = archiveInfo.filesInfo, &sem, this](int i) {
		const auto& fileName = fileNames[i]; // note generally (i != fid) due to ignore->Match(fi.fileName) filtering

		const auto it = filesInfo.find(fileName);
		assert(it != filesInfo.end());
		if (it->second.checksum != sha512::NULL_RAW_DIGEST)
			return;

		auto& fileBuffer = fileBuffers[ThreadPool::GetThreadNum()];
		fileBuffer.clear();

		sem.acquire();
		// note ar->FindFile() converts to lowercase
		numFilesHashed.fetch_add(static_cast<uint32_t>(ar->CalcHash(ar->FindFile(fileName), it->second.checksum, fileBuffer)));
		sem.release();
	});

	// stable sort by filename
	std::stable_sort(fileNames.begin(), fileNames.end(), [](const auto& lhs, const auto& rhs) {
		return std::lexicographical_compare(
			lhs.begin(), lhs.end(),
			rhs.begin(), rhs.end(),
			[](char c1, char c2) {
				return std::tolower(static_cast<unsigned char>(c1)) < std::tolower(static_cast<unsigned char>(c2));
			}
		);
	});

	// combine individual hashes, initialize to hash(name)
	for (size_t i = 0; i < fileNames.size(); i++) {
		auto fileName = fileNames[i];
		const auto filesInfoIt = archiveInfo.filesInfo.find(fileName);
		assert(filesInfoIt != archiveInfo.filesInfo.end());

		sha512::raw_digest fileNameHash{ 0 };
		{
			// we want the filename based hashing below to be case-independent
			StringToLowerInPlace(fileName);
			sha512::calc_digest(reinterpret_cast<const uint8_t*>(fileName.c_str()), fileName.size(), fileNameHash.data());
		}

		for (uint8_t j = 0; j < sha512::SHA_LEN; j++) {
			archiveInfo.checksum[j] ^= fileNameHash[j];
			archiveInfo.checksum[j] ^= filesInfoIt->second.checksum[j];
		}

		#if !defined(DEDICATED) && !defined(UNITSYNC)
		Watchdog::ClearTimer(WDT_MAIN);
		#endif
	}

	if (sdpArchive) {
		// makes no sense to store archiveInfo.filesInfo in the SDP entry
		// so copy to poolFilesInfo and empty archiveInfo.filesInfo
		for (uint32_t fid = 0; fid < ar->NumFiles(); ++fid) {
			const auto fi = ar->FileInfo(fid);

			if (ignore->Match(fi.fileName))
				continue;

			poolFilesInfo[fi.specialFileName] = archiveInfo.filesInfo[fi.fileName]; // populate the updated information back to poolFilesInfo
		}
		archiveInfo.filesInfo.clear();
	}
	else if (compressedArchive) {
		// makes no sense to to store archiveInfo.filesInfo for 7z/zip based archives
		// as these archives are likely immutable, the per file info is useless
		// in rare case of updating/overwriting the archive we will do full checksumming
		archiveInfo.filesInfo.clear();
	}
	else {
		for (auto it = archiveInfo.filesInfo.begin(); it != archiveInfo.filesInfo.end(); /*NOOP*/) {
			// cleanup files that got removed since the last cache update
			const auto fid = ar->FindFile(it->first);
			if (fid == ar->NumFiles()) {
				it = archiveInfo.filesInfo.erase(it);
			}
			// should never happen: read error?
			else if (const auto fi = ar->FileInfo(fid); fi.size == -1 || fi.modTime == 0) {
				it = archiveInfo.filesInfo.erase(it);
				assert(false);
				return false;
			}
			else {
				++it;
			}
		}
	}

	return true;
}


bool CArchiveScanner::ReadCacheData(const std::string& filename, bool loadOldVersion)
{
	std::lock_guard<decltype(scannerMutex)> lck(scannerMutex);
	if (!FileSystem::FileExists(filename)) {
		LOG_L(L_INFO, "[AS::%s] ArchiveCache %s doesn't exist", __func__, filename.c_str());
		return false;
	}

	LuaParser p(filename, SPRING_VFS_RAW, SPRING_VFS_BASE);
	if (!p.Execute()) {
		LOG_L(L_ERROR, "[AS::%s] failed to parse ArchiveCache: %s", __func__, p.GetErrorLog().c_str());
		return false;
	}

	const LuaTable archiveCacheTbl = p.GetRoot();

	// Do not load old version caches
	const int ver = archiveCacheTbl.GetInt("internalver", (INTERNAL_VER + 1));
	if (ver != INTERNAL_VER && !loadOldVersion)
		return false;

	static const auto ReadFileInfoMap = [](const LuaTable& filesInfoTbl, spring::unordered_map<std::string, FileInfo>& filesInfoMap) {
		for (int j = 1; filesInfoTbl.KeyExists(j); ++j) {
			const LuaTable fileInfoTbl = filesInfoTbl.SubTable(j);
			const auto fn = fileInfoTbl.GetString("fileName", "");
			if (fn.empty())
				continue;

			assert(!filesInfoMap.contains(fn));
			auto& val = filesInfoMap[fn];

			val.size = static_cast<decltype(val.size)>(std::stoll(fileInfoTbl.GetString("size", "-1")));
			val.modTime = static_cast<decltype(val.size)>(std::stoll(fileInfoTbl.GetString("modTime", "0")));
			val.checksum = sha512::read_digest(fileInfoTbl.GetString("checksum", ""));
		}
	};

	const LuaTable archivesTbl = archiveCacheTbl.SubTable("archives");
	for (int i = 1; archivesTbl.KeyExists(i); ++i) {
		const LuaTable curArchiveTbl = archivesTbl.SubTable(i);
		const LuaTable archivedTbl = curArchiveTbl.SubTable("archivedata");

		const std::string curArchiveName = curArchiveTbl.GetString("name", "");
		const std::string curArchiveNameLC = StringToLower(curArchiveName);
		const std::string hexDigestStr = curArchiveTbl.GetString("checksum", "");

		ArchiveInfo& ai = GetAddArchiveInfo(curArchiveNameLC);

		ai.origName 	   = curArchiveName;
		ai.path     	   = curArchiveTbl.GetString("path", "");
		ai.archiveDataPath = curArchiveTbl.GetString("archiveDataPath", "");

		// do not use LuaTable.GetInt() for integers: the engine's lua
		// library uses 32-bit floats to represent numbers, which can only
		// represent 2^24 consecutive integers
		ai.modified = static_cast<decltype(ai.modified)>(std::stoll(curArchiveTbl.GetString("modified", "0")));
		ai.modifiedArchiveData = static_cast<decltype(ai.modified)>(std::stoll(curArchiveTbl.GetString("modifiedArchiveData", "0")));

		const LuaTable filesInfoTbl = curArchiveTbl.SubTable("filesInfo");
		ReadFileInfoMap(filesInfoTbl, ai.filesInfo);

		ai.checksum = sha512::read_digest(hexDigestStr);

		ai.updated = false;
		ai.hashed = (ai.checksum != sha512::NULL_RAW_DIGEST);

		ai.archiveData = CArchiveScanner::ArchiveData(archivedTbl, true);
		if (ai.archiveData.IsMap()) {
			AddDependency(ai.archiveData.GetDependencies(), GetMapHelperContentName());
		} else if (ai.archiveData.IsGame()) {
			AddDependency(ai.archiveData.GetDependencies(), GetSpringBaseContentName());
		}
	}

	const LuaTable brokenArchivesTbl = archiveCacheTbl.SubTable("brokenArchives");
	for (int i = 1; brokenArchivesTbl.KeyExists(i); ++i) {
		const LuaTable curArchive = brokenArchivesTbl.SubTable(i);
		const std::string& name = StringToLower(curArchive.GetString("name", ""));

		BrokenArchive& ba = GetAddBrokenArchive(name);
		ba.name = name;
		ba.path = curArchive.GetString("path", "");
		ba.modified = static_cast<decltype(ba.modified)>(std::stoll(curArchive.GetString("modified", "0")));
		ba.updated = false;
		ba.problem = curArchive.GetString("problem", "unknown");
	}

	const LuaTable poolFilesTbl = archiveCacheTbl.SubTable("poolFiles");
	ReadFileInfoMap(poolFilesTbl, poolFilesInfo);

	isDirty = false;

	return true;
}

static inline void SafeStr(FILE* out, const char* prefix, const std::string& str)
{
	if (str.empty())
		return;

	if ((str.find_first_of("\\\"") != std::string::npos) || (str.find_first_of('\n') != std::string::npos )) {
		fprintf(out, "%s[[%s]],\n", prefix, str.c_str());
	} else {
		fprintf(out, "%s\"%s\",\n", prefix, str.c_str());
	}
}

void FilterDep(std::vector<std::string>& deps, const std::string& exclude)
{
	auto it = std::remove_if(deps.begin(), deps.end(), [&](const std::string& dep) { return (dep == exclude); });
	deps.erase(it, deps.end());
}

void CArchiveScanner::WriteCacheData(const std::string& filename)
{
	std::lock_guard<decltype(scannerMutex)> lck(scannerMutex);
	if (!isDirty)
		return;

	// First delete all outdated information
	{
		std::stable_sort(  archiveInfos.begin(),   archiveInfos.end(), [](const ArchiveInfo& a, const ArchiveInfo& b) { return (a.origName < b.origName); });
		std::stable_sort(brokenArchives.begin(), brokenArchives.end(), [](const BrokenArchive& a, const BrokenArchive& b) { return (a.name < b.name); });

		std::erase_if(  archiveInfos, [](const ArchiveInfo& i)   { return (!i.updated); });
		std::erase_if(brokenArchives, [](const BrokenArchive& i) { return (!i.updated); });

		archiveInfosIndex.clear();
		brokenArchivesIndex.clear();

		// rebuild index-maps
		for (const ArchiveInfo& ai: archiveInfos) {
			archiveInfosIndex.emplace(StringToLower(ai.origName), &ai - &archiveInfos[0]);
		}
		for (const BrokenArchive& bi: brokenArchives) {
			brokenArchivesIndex.emplace(bi.name, &bi - &brokenArchives[0]);
		}
	}

	// see if the cache contains pool files that don't exist anymore
	static constexpr size_t NUMFILES_VERIFICATION_THRESHOLD = 10000; // arbitrary number
	if (poolFilesInfo.size() >= NUMFILES_VERIFICATION_THRESHOLD)
	{
		// we go the complicated way, because we don't want to store pool root path in poolFilesInfo key
		// (to not blow up the size of the cache file)
		spring::unordered_set<std::string> allPoolRootDirs;
		for (const ArchiveInfo& ai : archiveInfos) {
			// lame way of detecting the archive type, that is not stored in ArchiveInfo
			if (ai.origName.find(".sdp") == std::string::npos)
				continue;

			allPoolRootDirs.emplace(CPoolArchive::GetPoolRootDirectory(ai.path + ai.origName));
		}

		const uint32_t seed = std::chrono::system_clock::now().time_since_epoch().count();
		std::mt19937 generator(seed);
		std::uniform_int_distribution<size_t> distribution(0, poolFilesInfo.size() - 1);
		auto startOffset = distribution(generator);

		auto st = poolFilesInfo.begin();
		std::advance(st, startOffset);
		auto it = st;

		// we only want to check if the file still exists, we don't check for size / modDate
		// this is checked in the checksum code anyway.
		const auto ExistenceTest = [&allPoolRootDirs = std::as_const(allPoolRootDirs)](const auto& it) {
			for (const auto& poolRootDir : allPoolRootDirs) {
				const auto fileName = CPoolArchive::GetPoolFilePath(poolRootDir, it->first);
				if (FileSystem::FileExists(fileName)) {
					return true;
				}
			}
			return false;
		};

		// can't spend too much time in this code, thus set the deadline and rely on
		// random luck and sheer amount of invocations to eventually remove most if not all
		// stale items from the pool cache
		static constexpr int64_t MAX_POOL_VERIFICATION_TIME = 1 * 1000;
		for (auto t0 = spring_now(), t1 = t0; (t1 - t0).toMilliSecsi() < MAX_POOL_VERIFICATION_TIME; t1 = spring_now()) {
			// cleanup files that got deleted in the meantime

			if (ExistenceTest(it))
				++it;
			else
				it = poolFilesInfo.erase(it);

			if (it == poolFilesInfo.end())
				it = poolFilesInfo.begin(); //rewind to the very start

			if (it == st)
				break; // everything got checked and we're back to the starting iterator
		}
	}

	FILE* out = fopen(filename.c_str(), "wt");
	if (out == nullptr) {
		LOG_L(L_ERROR, "[AS::%s] failed to write to \"%s\"!", __func__, filename.c_str());
		return;
	}

	auto WriteFileInfoMapBody = [out](const spring::unordered_map<std::string, FileInfo>& filesInfoMap, size_t numTabs) {
		const auto tabs = std::string(numTabs, '\t');
		for (const auto& [fn, fi] : filesInfoMap) {
			std::string tbl = fmt::format("{}{{ fileName = \"{}\", size = \"{}\", modTime = \"{}\", checksum = \"{}\" }},\n", tabs, fn, fi.size, fi.modTime, sha512::dump_digest(fi.checksum));
			fputs(tbl.c_str(), out);
		}
	};


	fprintf(out, "local archiveCache = {\n\n");
	fprintf(out, "\tinternalver = %i,\n\n", INTERNAL_VER);
	fprintf(out, "\tarchives = {  -- count = %u\n", uint32_t(archiveInfos.size()));

	for (const ArchiveInfo& arcInfo: archiveInfos) {
		sha512::raw_digest rawDigest;
		sha512::hex_digest hexDigest;

		rawDigest = arcInfo.checksum;
		sha512::dump_digest(rawDigest, hexDigest);

		fprintf(out, "\t\t{\n");
		SafeStr(out, "\t\t\tname = ",              arcInfo.origName);
		SafeStr(out, "\t\t\tpath = ",              arcInfo.path);
		fprintf(out, "\t\t\tmodified = \"%u\",\n", arcInfo.modified);
		fprintf(out, "\t\t\tchecksum = \"%s\",\n", hexDigest.data());
		SafeStr(out, "\t\t\treplaced = ",          arcInfo.replaced);

		if (!arcInfo.archiveDataPath.empty()) {
			SafeStr(out, "\t\t\tarchiveDataPath = ",              arcInfo.archiveDataPath);
			fprintf(out, "\t\t\tmodifiedArchiveData = \"%u\",\n", arcInfo.modifiedArchiveData);
		}

		if (!arcInfo.filesInfo.empty()) {
			fprintf(out, "\t\t\tfilesInfo = {\n");
			WriteFileInfoMapBody(arcInfo.filesInfo, 4);
			fprintf(out, "\t\t\t},\n");
		}

		// mod info?
		const ArchiveData& archData = arcInfo.archiveData;
		if (!archData.GetName().empty()) {
			fprintf(out, "\t\t\tarchivedata = {\n");

			for (const auto& ii: archData.GetInfo()) {
				if (ii.second.valueType == INFO_VALUE_TYPE_STRING) {
					SafeStr(out, std::string("\t\t\t\t" + ii.first + " = ").c_str(), ii.second.valueTypeString);
				} else {
					fprintf(out, "\t\t\t\t%s = %s,\n", ii.first.c_str(), ii.second.GetValueAsString(false).c_str());
				}
			}

			std::vector<std::string> deps = archData.GetDependencies();
			if (archData.IsMap()) {
				FilterDep(deps, GetMapHelperContentName());
			} else if (archData.IsGame()) {
				FilterDep(deps, GetSpringBaseContentName());
			}

			if (!deps.empty()) {
				fprintf(out, "\t\t\t\tdepend = {\n");
				for (const auto& dep: deps) {
					SafeStr(out, "\t\t\t\t\t", dep);
				}
				fprintf(out, "\t\t\t\t},\n");
			}
			fprintf(out, "\t\t\t},\n");
		}

		fprintf(out, "\t\t},\n");
	}

	fprintf(out, "\t},\n\n"); // close 'archives'
	fprintf(out, "\tbrokenArchives = {  -- count = %u\n", uint32_t(brokenArchives.size()));

	for (const BrokenArchive& ba: brokenArchives) {
		fprintf(out, "\t\t{\n");
		SafeStr(out, "\t\t\tname = ", ba.name);
		SafeStr(out, "\t\t\tpath = ", ba.path);
		fprintf(out, "\t\t\tmodified = \"%u\",\n", ba.modified);
		SafeStr(out, "\t\t\tproblem = ", ba.problem);
		fprintf(out, "\t\t},\n");
	}

	fprintf(out, "\t},\n"); // close 'brokenArchives'

	// Information about files in the pool
	if (!poolFilesInfo.empty()) {
		fprintf(out, "\tpoolFiles = {  -- count = %u\n", uint32_t(poolFilesInfo.size()));
		WriteFileInfoMapBody(poolFilesInfo, 2);
		fprintf(out, "\t},\n"); // close 'poolFiles'
	}

	fprintf(out, "}\n\n"); // close 'archiveCache'
	fprintf(out, "return archiveCache\n");

	if (fclose(out) == EOF)
		LOG_L(L_ERROR, "[AS::%s] failed to write to \"%s\"!", __func__, filename.c_str());

	isDirty = false;
}


static void sortByName(std::vector<CArchiveScanner::ArchiveData>& data)
{
	std::stable_sort(data.begin(), data.end(), [](const CArchiveScanner::ArchiveData& a, const CArchiveScanner::ArchiveData& b) {
		return (a.GetNameVersioned() < b.GetNameVersioned());
	});
}

std::vector<CArchiveScanner::ArchiveData> CArchiveScanner::GetPrimaryMods() const
{
	std::lock_guard<decltype(scannerMutex)> lck(scannerMutex);

	std::vector<ArchiveData> ret;
	ret.reserve(archiveInfos.size());

	for (const ArchiveInfo& ai: archiveInfos) {
		const ArchiveData& aid = ai.archiveData;

		if ((!aid.GetName().empty()) && (aid.GetModType() == modtype::primary)) {
			// Add the archive the mod is in as the first dependency
			ArchiveData md = aid;
			md.GetDependencies().insert(md.GetDependencies().begin(), ai.origName);
			ret.push_back(md);
		}
	}

	sortByName(ret);
	return ret;
}


std::vector<CArchiveScanner::ArchiveData> CArchiveScanner::GetAllMods() const
{
	std::lock_guard<decltype(scannerMutex)> lck(scannerMutex);

	std::vector<ArchiveData> ret;
	ret.reserve(archiveInfos.size());

	for (const ArchiveInfo& ai: archiveInfos) {
		const ArchiveData& aid = ai.archiveData;

		if ((!aid.GetName().empty()) && aid.IsGame()) {
			// Add the archive the mod is in as the first dependency
			ArchiveData md = aid;
			md.GetDependencies().insert(md.GetDependencies().begin(), ai.origName);
			ret.push_back(md);
		}
	}

	sortByName(ret);
	return ret;
}


std::vector<CArchiveScanner::ArchiveData> CArchiveScanner::GetAllArchives() const
{
	std::lock_guard<decltype(scannerMutex)> lck(scannerMutex);

	std::vector<ArchiveData> ret;
	ret.reserve(archiveInfos.size());

	for (const ArchiveInfo& ai: archiveInfos) {
		const ArchiveData& aid = ai.archiveData;

		// Add the archive the mod is in as the first dependency
		ArchiveData md = aid;
		md.GetDependencies().insert(md.GetDependencies().begin(), ai.origName);
		ret.push_back(md);
	}

	sortByName(ret);
	return ret;
}


std::vector<std::string> CArchiveScanner::GetAllArchivesUsedBy(const std::string& rootArchive) const
{
	LOG_S(LOG_SECTION_ARCHIVESCANNER, "GetArchives: %s", rootArchive.c_str());

	// VectorInsertUnique'ing via AddDependency can become a performance hog
	// for very long dependency chains, prefer to sort and remove duplicates
	const auto& NameCmp = [](const std::pair<std::string, size_t>& a, const std::pair<std::string, size_t>& b) { return (a.first  < b.first ); };
	const auto& IndxCmp = [](const std::pair<std::string, size_t>& a, const std::pair<std::string, size_t>& b) { return (a.second < b.second); };

	std::lock_guard<decltype(scannerMutex)> lck(scannerMutex);

	std::vector<          std::string         > retArchives;
	std::vector<std::pair<std::string, size_t>> tmpArchives[2];

	std::deque<std::string> archiveQueue = {rootArchive};

	retArchives.reserve(8);
	tmpArchives[0].reserve(8);
	tmpArchives[1].reserve(8);

	while (!archiveQueue.empty()) {
		// protect against circular dependencies; worst case is if all archives form one huge chain
		if (archiveQueue.size() > archiveInfos.size())
			break;

		const std::string& resolvedName = ArchiveNameResolver::GetGame(archiveQueue.front());

		archiveQueue.pop_front();

		const ArchiveInfo* archiveInfo = nullptr;

		const auto CanAddSubDependencies = [&](const std::string& resolvedName) -> const ArchiveInfo* {
			#ifdef UNITSYNC
			// add unresolved deps for unitsync so it still shows this file
			const auto HandleUnresolvedDep = [&tmpArchives](const std::string& archName) { tmpArchives[0].emplace_back(archName, tmpArchives[0].size()); return true; };
			#else
			const auto HandleUnresolvedDep = [&tmpArchives](const std::string& archName) { (void) archName; return false; };
			#endif

			const std::string& lowerCaseName = StringToLower(ArchiveFromName(resolvedName));

			auto aii = archiveInfosIndex.find(lowerCaseName);
			auto aij = aii;

			const ArchiveInfo* ai = nullptr;

			if (aii == archiveInfosIndex.end()) {
				if (!HandleUnresolvedDep(lowerCaseName))
					throw content_error("Dependent archive \"" + lowerCaseName + "\" (resolved to \"" + resolvedName + "\") not found");

				return nullptr;
			}

			ai = &archiveInfos[aii->second];

			// check if this archive has an unresolved replacement
			while (!ai->replaced.empty()) {
				if ((aii = archiveInfosIndex.find(ai->replaced)) == archiveInfosIndex.end()) {
					if (!HandleUnresolvedDep(lowerCaseName))
						throw content_error("Replacement \"" + ai->replaced + "\" for archive \"" + resolvedName + "\" not found");

					return nullptr;
				}

				aij = aii;
				ai = &archiveInfos[aij->second];
			}

			return ai;
		};


		if ((archiveInfo = CanAddSubDependencies(resolvedName)) == nullptr)
			continue;

		tmpArchives[0].emplace_back(archiveInfo->archiveData.GetNameVersioned(), tmpArchives[0].size());

		// expand dependencies in depth-first order
		for (const std::string& archiveDep: archiveInfo->archiveData.GetDependencies()) {
			assert(archiveDep != rootArchive);
			assert(archiveDep != tmpArchives[0][tmpArchives[0].size() - 1].first);
			archiveQueue.push_front(archiveDep);
		}
	}

	std::stable_sort(tmpArchives[0].begin(), tmpArchives[0].end(), NameCmp);

	// filter out any duplicate dependencies
	for (auto& archiveEntry: tmpArchives[0]) {
		if (tmpArchives[1].empty() || archiveEntry.first != tmpArchives[1][tmpArchives[1].size() - 1].first) {
			tmpArchives[1].emplace_back(std::move(archiveEntry.first), archiveEntry.second);
		}
	}

	// resort in original traversal order so overrides work as expected
	std::stable_sort(tmpArchives[1].begin(), tmpArchives[1].end(), IndxCmp);

	for (auto& archiveEntry: tmpArchives[1]) {
		retArchives.emplace_back(std::move(archiveEntry.first));
	}

	return retArchives;
}


std::vector<std::string> CArchiveScanner::GetMaps() const
{
	std::vector<std::string> ret;

	for (const ArchiveInfo& ai: archiveInfos) {
		const ArchiveData& ad = ai.archiveData;

		if (!(ad.GetName().empty()) && ad.IsMap())
			ret.push_back(ad.GetNameVersioned());
	}

	return ret;
}

std::string CArchiveScanner::MapNameToMapFile(const std::string& versionedMapName) const
{
	// Convert map name to map archive
	const auto pred = [&](const decltype(archiveInfos)::value_type& p) { return (p.archiveData.GetNameVersioned() == versionedMapName); };
	const auto iter = std::find_if(archiveInfos.cbegin(), archiveInfos.cend(), pred);

	if (iter != archiveInfos.cend())
		return (iter->archiveData.GetMapFile());

	LOG_SL(LOG_SECTION_ARCHIVESCANNER, L_WARNING, "map file of %s not found", versionedMapName.c_str());
	return versionedMapName;
}

void DumpArchiveChecksum(const std::string& lcName, const sha512::raw_digest& cs) {
#if ACRHIVE_CHECKSUM_DUMP == 1
	{
		sha512::hex_digest hexHash;
		hexHash.fill(0);

		sha512::dump_digest(cs, hexHash);

		LOG_L(L_INFO, "[CAS::GASCB] Archive file=\"%s\" cs=\"%s\"", lcName.c_str(), &hexHash[0]);
	}
#endif
}

sha512::raw_digest CArchiveScanner::GetArchiveSingleChecksumBytes(const std::string& filePath)
{
	std::lock_guard<decltype(scannerMutex)> lck(scannerMutex);

	// compute checksum for archive only when it is actually loaded by e.g. PreGame or LuaVFS
	// (this updates its ArchiveInfo iff !CheckCachedData and marks the scanner as dirty s.t.
	// cache will be rewritten on reload/shutdown)
	ScanArchive(filePath, true);

	const std::string lcName = StringToLower(FileSystem::GetFilename(filePath));
	const auto aiIter = archiveInfosIndex.find(lcName);

	sha512::raw_digest checksum = sha512::NULL_RAW_DIGEST;

	if (aiIter == archiveInfosIndex.end()) {
		DumpArchiveChecksum(lcName, checksum); //cs is 0
		return checksum;
	}

	checksum = archiveInfos[aiIter->second].checksum;
	DumpArchiveChecksum(lcName, checksum);
	return checksum;
}

sha512::raw_digest CArchiveScanner::GetArchiveCompleteChecksumBytes(const std::string& name)
{
	sha512::raw_digest checksum{0};

	for (const std::string& depName: GetAllArchivesUsedBy(name)) {
		const std::string& archiveName = ArchiveFromName(depName);
		const std::string  archivePath = GetArchivePath(archiveName) + archiveName;

		const sha512::raw_digest archiveChecksum = GetArchiveSingleChecksumBytes(archivePath);

		for (uint8_t i = 0; i < sha512::SHA_LEN; i++) {
			checksum[i] ^= archiveChecksum[i];
		}
	}

	return checksum;
}

static constexpr sha512::raw_digest EMPTY_DIGEST = {0x80, }; // 1000...000 hex
void CArchiveScanner::CheckArchive(
	const std::string& name,
	const sha512::raw_digest& serverChecksum,
	      sha512::raw_digest& clientChecksum
) {
	/* Dedicated servers often don't actually have the content,
	 * but this is fine as they just relay traffic - don't warn. */
	if (serverChecksum == EMPTY_DIGEST)
		return;

	if ((clientChecksum = GetArchiveCompleteChecksumBytes(name)) == serverChecksum)
		return;

	sha512::hex_digest serverChecksumHex;
	sha512::hex_digest clientChecksumHex;
	sha512::dump_digest(serverChecksum, serverChecksumHex);
	sha512::dump_digest(clientChecksum, clientChecksumHex);

	char msg[1024];
	sprintf(
		msg,
		"Archive %s (checksum %s) differs from the host's copy (checksum %s). "
		"This may be caused by a corrupted download or there may even be two "
		"different versions in circulation. Make sure you and the host have installed "
		"the chosen archive and its dependencies and consider redownloading it.",
		name.c_str(), clientChecksumHex.data(), serverChecksumHex.data());

	throw content_error(msg);
}

std::string CArchiveScanner::GetArchivePath(const std::string& archiveName) const
{
	std::lock_guard<decltype(scannerMutex)> lck(scannerMutex);

	const auto aii = archiveInfosIndex.find(StringToLower(FileSystem::GetFilename(archiveName)));

	if (aii == archiveInfosIndex.end())
		return "";

	return archiveInfos[aii->second].path;
}

std::string CArchiveScanner::NameFromArchive(const std::string& archiveName) const
{
	std::lock_guard<decltype(scannerMutex)> lck(scannerMutex);

	const auto aii = archiveInfosIndex.find(StringToLower(archiveName));

	if (aii != archiveInfosIndex.end())
		return (archiveInfos[aii->second].archiveData.GetNameVersioned());

	return archiveName;
}


std::string CArchiveScanner::GameHumanNameFromArchive(const std::string& archiveName) const
{
	return (ArchiveNameResolver::GetGame(NameFromArchive(archiveName)));
}

std::string CArchiveScanner::MapHumanNameFromArchive(const std::string& archiveName) const
{
	return (ArchiveNameResolver::GetMap(NameFromArchive(archiveName)));
}


std::string CArchiveScanner::ArchiveFromName(const std::string& versionedName) const
{
	std::lock_guard<decltype(scannerMutex)> lck(scannerMutex);

	const auto pred = [&](const decltype(archiveInfos)::value_type& p) { return (p.archiveData.GetNameVersioned() == versionedName); };
	const auto iter = std::find_if(archiveInfos.cbegin(), archiveInfos.cend(), pred);

	if (iter != archiveInfos.cend())
		return iter->origName;

	return versionedName;
}

CArchiveScanner::ArchiveData CArchiveScanner::GetArchiveData(const std::string& versionedName) const
{
	std::lock_guard<decltype(scannerMutex)> lck(scannerMutex);

	const auto pred = [&](const decltype(archiveInfos)::value_type& p) { return (p.archiveData.GetNameVersioned() == versionedName); };
	const auto iter = std::find_if(archiveInfos.cbegin(), archiveInfos.cend(), pred);

	if (iter != archiveInfos.cend())
		return iter->archiveData;

	return {};
}


CArchiveScanner::ArchiveData CArchiveScanner::GetArchiveDataByArchive(const std::string& archive) const
{
	std::lock_guard<decltype(scannerMutex)> lck(scannerMutex);

	const auto aii = archiveInfosIndex.find(StringToLower(archive));

	if (aii != archiveInfosIndex.end())
		return archiveInfos[aii->second].archiveData;

	return {};
}

int CArchiveScanner::GetMetaFileClass(const std::string& filePath)
{
	const std::string& lowerFilePath = StringToLower(filePath);
	// const std::string& ext = FileSystem::GetExtension(lowerFilePath);
	const auto it = metaFileClasses.find(lowerFilePath);

	if (it != metaFileClasses.end())
		return (it->second);

//	if (ext == "smf") // to generate minimap
//		return 1;

	for (const auto& p: metaDirClasses) {
		if (StringStartsWith(lowerFilePath, p.first))
			return (p.second);
	}

	return 0;
}

