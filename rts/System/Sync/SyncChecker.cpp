/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#ifdef SYNCCHECK

#include "SyncChecker.h"

// This cannot be included in the header file (SyncChecker.h) because include conflicts will occur.
#include "System/Threading/ThreadPool.h"


unsigned CSyncChecker::g_checksum;
int CSyncChecker::inSyncedCode;

#ifdef SYNC_HISTORY
unsigned CSyncChecker::nextHistoryIndex = 0;
unsigned CSyncChecker::logs[MAX_SYNC_HISTORY];

void CSyncChecker::LogHistory()
{
	logs[nextHistoryIndex++] = g_checksum;
	if (nextHistoryIndex == MAX_SYNC_HISTORY) {
		nextHistoryIndex = 0;
		//LOG("[Sync::Checker] HISTORY chksum=%u\n", g_checksum);
		printf("[Sync::Checker] HISTORY\n");
	}
}
#endif // SYNC_HISTORY

void CSyncChecker::debugSyncCheckThreading()
{
    assert(ThreadPool::GetThreadNum() == 0);
}

#endif // SYNCDEBUG
