/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <algorithm>
#include <climits>
#include <cstring>

#include "System/TimeProfiler.h"
#include "System/GlobalRNG.h"
#include "System/StringHash.h"
#include "System/Log/ILog.h"
#include "System/Threading/SpringThreading.h"

#ifdef THREADPOOL
	#include "System/Threading/ThreadPool.h"
#endif

using ProfileMutexType = spring::mutex; //spring::spinlock
using HashNamMutexType = spring::mutex; //spring::spinlock

static ProfileMutexType profileMutex;
static HashNamMutexType hashToNameMutex;
static spring::unordered_map<unsigned, std::string> hashToName;
static spring::unordered_map<unsigned, int> refCounters;

static CGlobalUnsyncedRNG profileColorRNG;

const std::array<CTimeProfiler::ProfileSortFunc, CTimeProfiler::SortType::ST_COUNT> CTimeProfiler::SortingFunctions = {
	[](const TimeRecordPair& a, const TimeRecordPair& b) { return (a.first          < b.first         ); }, // ST_ALPHABETICAL = 0,
	[](const TimeRecordPair& a, const TimeRecordPair& b) { return (a.second.total   > b.second.total  ); }, // ST_TOTALTIME    = 1,
	[](const TimeRecordPair& a, const TimeRecordPair& b) { return (a.second.stats.y > b.second.stats.y); }, // ST_CURRENTTIME  = 2,
	[](const TimeRecordPair& a, const TimeRecordPair& b) { return (a.second.stats.z > b.second.stats.z); }, // ST_MAXTIME      = 3,
	[](const TimeRecordPair& a, const TimeRecordPair& b) { return (a.second.stats.x > b.second.stats.x); }, // ST_LAG          = 4,
};


spring_time BasicTimer::GetDuration() const
{
	return spring_difftime(spring_gettime(), startTime);
}

ScopedTimer::ScopedTimer(const unsigned _nameHash, bool _autoShowGraph, bool _specialTimer)
	: BasicTimer(_nameHash)

	// Game::SendClientProcUsage depends on "Sim" and "Draw" percentages, BenchMark on "Lua"
	// note that address-comparison is intended here, timer names are (and must be) literals
	, autoShowGraph(_autoShowGraph)
	, specialTimer(_specialTimer)
{
	auto iter = refCounters.find(nameHash);

	if (iter == refCounters.end())
		iter = refCounters.insert(std::pair<unsigned, int>(nameHash, 0)).first;

	++(iter->second);
}

ScopedTimer::~ScopedTimer()
{
	// no avoiding a second lookup since iterators can be invalidated with unordered_map
	auto iter = refCounters.find(nameHash);

	assert(iter != refCounters.end());
	assert(iter->second > 0);

	if (--(iter->second) == 0) {
		CTimeProfiler::GetInstance().AddTime(nameHash, startTime, GetDuration(), autoShowGraph, specialTimer, false);
	}
}



ScopedOnceTimer::ScopedOnceTimer(const char* timerName, const char* timerFrmt): startTime(spring_gettime())
{
	strncpy(name, timerName, sizeof(name));
	strncpy(frmt, timerFrmt, sizeof(frmt));

	name[sizeof(name) - 1] = 0;
	frmt[sizeof(frmt) - 1] = 0;
}

ScopedOnceTimer::ScopedOnceTimer(const std::string& timerName, const char* timerFrmt): startTime(spring_gettime())
{
	strncpy(name, timerName.c_str(), sizeof(name));
	strncpy(frmt, timerFrmt        , sizeof(frmt));

	name[sizeof(name) - 1] = 0;
	frmt[sizeof(frmt) - 1] = 0;
}

ScopedOnceTimer::~ScopedOnceTimer()
{
	LOG(frmt, __func__, name, int(GetDuration().toMilliSecsi()));
}

spring_time ScopedOnceTimer::GetDuration() const
{
	return spring_difftime(spring_gettime(), startTime);
}



ScopedMtTimer::ScopedMtTimer(unsigned _nameHash, bool _autoShowGraph)
	: BasicTimer(_nameHash)
	, autoShowGraph(_autoShowGraph)
{
}

ScopedMtTimer::~ScopedMtTimer()
{
	CTimeProfiler::GetInstance().AddTime(nameHash, startTime, GetDuration(), autoShowGraph, false, true);
}



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTimeProfiler::CTimeProfiler()
{
	// self
	RegisterTimer("Misc::Profiler::AddTime");
	// specials (conditional on LuaContextData)
	RegisterTimer("Lua::Callins::Synced");
	RegisterTimer("Lua::Callins::Unsynced");
	RegisterTimer("Lua::CollectGarbage::Synced");
	RegisterTimer("Lua::CollectGarbage::Unsynced");
	ResetState();
}

#if 1
CTimeProfiler::~CTimeProfiler() = default;
#else
CTimeProfiler::~CTimeProfiler()
{
	// should not be needed, destructor runs after main returns and all threads are gone
	std::lock_guard<ProfileMutexType> lock(profileMutex);
}
#endif


CTimeProfiler& CTimeProfiler::GetInstance()
{
	static CTimeProfiler tp;
	return tp;
}

bool CTimeProfiler::RegisterTimer(const char* timerName)
{
	const unsigned nameHash = hashString(timerName);

	std::lock_guard<HashNamMutexType> lock(hashToNameMutex);

	const auto iter = hashToName.find(nameHash);

	if (iter == hashToName.end()) {
		hashToName.emplace(nameHash, timerName);
		return true;
	}
	if (iter->second == timerName)
		return true;

	LOG_L(L_ERROR, "[%s] timer hash collision: %s <=> %s", __func__, timerName, iter->second.c_str());
	assert(false);
	return false;
}

bool CTimeProfiler::UnRegisterTimer(const char* timerName)
{
	const unsigned nameHash = hashString(timerName);

	std::lock_guard<HashNamMutexType> lock(hashToNameMutex);

	const auto iter = hashToName.find(nameHash);

	if (iter == hashToName.end())
		return false;

	hashToName.erase(iter);
	return true;
}


void CTimeProfiler::ResetState() {
	// grab lock; ThreadPool workers might already be running SCOPED_MT_TIMER
	std::lock_guard<ProfileMutexType> lock(profileMutex);

	profiles.clear();
	profiles.reserve(128);
	sortedProfiles.clear();
	#ifdef THREADPOOL
	threadProfiles.clear();
	threadProfiles.resize(ThreadPool::GetMaxThreads());
	#endif

	profileColorRNG.Seed(spring_tomsecs(lastBigUpdate = spring_gettime()));

	currentPosition = 0;
	resortProfiles = 0;

	enabled = false;
}

void CTimeProfiler::ToggleLock(bool lock)
{
	if (lock) {
		profileMutex.lock();
	} else {
		profileMutex.unlock();
	}
}


void CTimeProfiler::Update()
{
	if (!enabled) {
		UpdateRaw();
		ResortProfilesRaw();
		RefreshProfilesRaw();
		return;
	}

	// FIXME: non-locking threadsafe
	std::lock_guard<ProfileMutexType> lock(profileMutex);

	if (sortingType != ST_ALPHABETICAL)
		++resortProfiles;

	UpdateRaw();
	ResortProfilesRaw();
	RefreshProfilesRaw();
	// Now cleanup old thread profiles, no need to do it if
	// disabled since won't be accepting data.
	CleanupOldThreadProfiles();
}

void CTimeProfiler::UpdateRaw()
{
	currentPosition += 1;
	currentPosition &= (TimeRecord::numFrames - 1);

	for (auto& pi: profiles) {
		pi.second.frames[currentPosition] = spring_notime;
	}

	const spring_time curTime = spring_gettime();
	const float timeDiff = spring_diffmsecs(curTime, lastBigUpdate);

	if (timeDiff > 500.0f) {
		// update percentages and peaks twice every second
		for (auto& pi: profiles) {
			auto& p = pi.second;

			p.stats.y = spring_tomsecs(p.current) / timeDiff;
			p.current = spring_notime;

			p.newLagPeak = false;
			p.newPeak = (p.stats.y > p.stats.z);

			if (!p.newPeak)
				continue;

			p.stats.z = p.stats.y;

		}

		lastBigUpdate = curTime;
	}

	if (curTime.toSecsi() % 6 == 0) {
		for (auto& pi: profiles) {
			(pi.second).stats.x *= 0.5f;
		}
	}
}

void CTimeProfiler::ResortProfilesRaw()
{
	if (resortProfiles > 0) {
		resortProfiles = 0;

		sortedProfiles.clear();
		sortedProfiles.reserve(profiles.size());

		// either caller already has lock, or we are disabled and thread-safe
		{
			std::lock_guard<HashNamMutexType> lock(hashToNameMutex);

			for (const auto& profile: profiles) {
				const auto iter = hashToName.find(profile.first);

				if (iter == hashToName.end()) {
					LOG_L(L_ERROR, "[%s] timer with hash %u wasn't registered", __func__, profile.first);
					assert(false);
					hashToName.emplace(profile.first, "???");
					continue;
				}

				sortedProfiles.emplace_back(iter->second, profile.second);
			}
		}

		const auto& sortFunc = SortingFunctions[sortingType];
		std::sort(sortedProfiles.begin(), sortedProfiles.end(), sortFunc);
	}
}


void CTimeProfiler::RefreshProfiles()
{
	// ProfileDrawer calls this, and is only enabled when we are
	assert(enabled);

	// lock so nothing modifies *unsorted* profiles during the refresh
	std::lock_guard<ProfileMutexType> lock(profileMutex);

	RefreshProfilesRaw();
}

void CTimeProfiler::RefreshProfilesRaw()
{
	// either called from ProfileDrawer or from Update; the latter
	// makes the "/debuginfo profiling" command work when disabled
	for (auto& sortedProfile: sortedProfiles) {
		TimeRecord& rec = sortedProfile.second;

		const bool showGraph = rec.showGraph;

		rec = profiles[hashString(sortedProfile.first.c_str())];
		rec.showGraph = showGraph;
	}
}

void CTimeProfiler::CleanupOldThreadProfiles()
{
	#ifdef THREADPOOL
	const spring_time curTime = spring_gettime();
	const spring_time maxTime = spring_secs(MAX_THREAD_HIST_TIME);
	const size_t numThreads = std::min(threadProfiles.size(), (size_t)ThreadPool::GetNumThreads());
	size_t i = 0;

	for (auto& threadProf: threadProfiles) {
		if (i++ >= numThreads) break;
		while (!threadProf.empty() && (curTime - threadProf.front().second) > maxTime) {
			threadProf.pop_front();
		}
	}
	#endif
}

const CTimeProfiler::TimeRecord& CTimeProfiler::GetTimeRecord(const char* name) const
{
	// if disabled, only special timers can pass AddTime
	// all of those are non-threaded, so no need to lock
	if (!enabled)
		return (GetTimeRecordRaw(name));

	std::lock_guard<ProfileMutexType> lock(profileMutex);

	return (GetTimeRecordRaw(name));
}


void CTimeProfiler::AddTime(
	const unsigned nameHash,
	const spring_time startTime,
	const spring_time deltaTime,
	const bool showGraph,
	const bool specialTimer,
	const bool threadTimer
) {
	const spring_time t0 = spring_now();

	if (!enabled) {
		if (!specialTimer)
			return;

		assert(!threadTimer);
		AddTimeRaw(nameHash, startTime, deltaTime, showGraph, threadTimer);
		AddTimeRaw(hashString("Misc::Profiler::AddTime"), t0, spring_now() - t0, false, false);
		return;
	}

	// acquire lock at the start; one inserting thread could
	// cause a profile rehash and invalidate <pi> for another
	std::lock_guard<ProfileMutexType> lock(profileMutex);

	AddTimeRaw(nameHash, startTime, deltaTime, showGraph, threadTimer);
	AddTimeRaw(hashString("Misc::Profiler::AddTime"), t0, spring_now() - t0, false, false);
}

void CTimeProfiler::AddTimeRaw(
	const unsigned nameHash,
	const spring_time startTime,
	const spring_time deltaTime,
	const bool showGraph,
	const bool threadTimer
) {
#ifdef THREADPOOL
	if (threadTimer)
		threadProfiles[ThreadPool::GetThreadNum()].emplace_back(startTime, spring_gettime());
#endif

	auto pi = profiles.find(nameHash);
	auto& p = (pi != profiles.end()) ? pi->second: profiles[nameHash];

	// these are 0 if just created, works for both paths
	p.total   += deltaTime;
	p.current += deltaTime;

	p.newLagPeak = (p.stats.x > 0.0f && deltaTime.toMilliSecsf() > p.stats.x);
	p.stats.x    = std::max(p.stats.x, deltaTime.toMilliSecsf());

	if (pi != profiles.end()) {
		// profile already exists, add dt
		p.frames[currentPosition] += deltaTime;
	} else {
		// new profile, new color
		p.color.x = profileColorRNG.NextFloat();
		p.color.y = profileColorRNG.NextFloat();
		p.color.z = profileColorRNG.NextFloat();
		p.showGraph = showGraph;

		resortProfiles += 1;
	}
}

void CTimeProfiler::PrintProfilingInfo() const
{
	if (sortedProfiles.empty())
		return;

	LOG("%35s|%18s|%s", "Part", "Total Time", "Time of the last 0.5s");

	for (const auto& sortedProfile: sortedProfiles) {
		const std::string& name = sortedProfile.first;
		const TimeRecord& tr = sortedProfile.second;

		LOG("%35s %16.2fms %5.2f%%", name.c_str(), tr.total.toMilliSecsf(), tr.stats.y * 100);
	}
}

