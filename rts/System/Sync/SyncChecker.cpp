/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#ifdef SYNCCHECK

#include "SyncChecker.h"

// This cannot be included in the header file (SyncChecker.h) because include conflicts will occur.
#include "System/Threading/ThreadPool.h"
#include "System/HashSpec.h"


unsigned CSyncChecker::g_checksum;
unsigned CSyncChecker::g_prevChecksum;
int CSyncChecker::inSyncedCode;

void CSyncChecker::NewFrame()
{
	g_checksum = 0xfade1eaf;
#ifdef SYNC_HISTORY
	LogHistory();
#endif // SYNC_HISTORY
}

void CSyncChecker::debugSyncCheckThreading()
{
	assert(ThreadPool::GetThreadNum() == 0);
}

void CSyncChecker::Sync(uint32_t val)
{
#ifdef DEBUG_SYNC_MT_CHECK
	// Sync calls should not be occurring in multi-threaded sections
	debugSyncCheckThreading();
#endif
	g_checksum = spring::hash_combine(val, g_checksum);
	//LOG("[Sync::Checker] chksum=%u\n", g_checksum);

#ifdef SYNC_HISTORY
	LogHistory();
#endif // SYNC_HISTORY
}

void CSyncChecker::Sync(const void* p, unsigned size)
{
#ifdef DEBUG_SYNC_MT_CHECK
	// Sync calls should not be occurring in multi-threaded sections
	debugSyncCheckThreading();
#endif
	// most common cases first, make it easy for compiler to optimize for it
	// simple xor is not enough to detect multiple zeroes, e.g.
	g_checksum = spring::LiteHash(p, size, g_checksum);
	//LOG("[Sync::Checker] chksum=%u\n", g_checksum);

#ifdef SYNC_HISTORY
	LogHistory();
#endif // SYNC_HISTORY
}

#ifdef SYNC_HISTORY

unsigned CSyncChecker::nextHistoryIndex = 0;
unsigned CSyncChecker::nextFrameIndex = 0;
std::array<unsigned, MAX_SYNC_HISTORY> CSyncChecker::logs;
std::array<unsigned, MAX_SYNC_HISTORY_FRAMES> CSyncChecker::logFrames;

void CSyncChecker::NewGameFrame()
{
	logFrames[nextFrameIndex++] = nextHistoryIndex;
	if (nextFrameIndex == MAX_SYNC_HISTORY_FRAMES)
		nextFrameIndex = 0;
}

void CSyncChecker::LogHistory()
{
	logs[nextHistoryIndex++] = g_checksum;
	if (nextHistoryIndex == MAX_SYNC_HISTORY)
		nextHistoryIndex = 0;
}

std::tuple<unsigned, unsigned, unsigned*> CSyncChecker::GetFrameHistory(unsigned rewindFrames)
{
	int endFrameIndex = nextFrameIndex - rewindFrames;
	int startFrameIndex = endFrameIndex - 1;

	if (endFrameIndex < 0)
		endFrameIndex = MAX_SYNC_HISTORY_FRAMES + endFrameIndex;
	if (startFrameIndex < 0)
		startFrameIndex = MAX_SYNC_HISTORY_FRAMES + startFrameIndex;

	return std::make_tuple(logFrames[startFrameIndex], logFrames[endFrameIndex], logs.data());
}

#endif // SYNC_HISTORY

#endif // SYNCCHECK
