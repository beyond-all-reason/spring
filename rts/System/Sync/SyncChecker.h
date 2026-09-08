/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SYNCCHECKER_H
#define SYNCCHECKER_H

#include "System/SpringHash.h"

#include <cassert>
#include <array>

static constexpr size_t MAX_SYNC_HISTORY = 2500000; // 10MB, ~= 10 seconds of typical midgame
static constexpr size_t MAX_SYNC_HISTORY_FRAMES = 1000;

/**
 * @brief sync checker class
 *
 * A Lightweight sync debugger that just keeps a running checksum over all
 * assignments to synced variables.
 */
class CSyncChecker {

	public:
		/**
		 * Whether one thread (doesn't have to be the current thread!!!) is currently processing a SimFrame.
		 */
		static bool InSyncedCode()    { return (inSyncedCode > 0); }
		static void EnterSyncedCode() { ++inSyncedCode; }
		static void LeaveSyncedCode() { assert(InSyncedCode()); --inSyncedCode; }

		/**
		 * Keeps a running checksum over all assignments to synced variables.
		 */
		static unsigned GetChecksum() { return g_checksum; }
		static unsigned GetPrevChecksum() { return g_prevChecksum; }
		static void SetPrevChecksum(unsigned v) { g_prevChecksum = v; }
		static void NewFrame();
		static void debugSyncCheckThreading();
		static void Sync(uint32_t val);
		static void Sync(const void* p, unsigned size);
		#ifdef SYNC_HISTORY
		static std::tuple<unsigned, unsigned, unsigned*> GetFrameHistory(unsigned rewindFrames);
		static std::pair<unsigned, unsigned*> GetHistory() { return std::make_pair(nextHistoryIndex, logs.data()); };
		static void NewGameFrame();
		#endif // SYNC_HISTORY

	private:

		/**
		 * The sync checksum
		 */
		static unsigned g_checksum;

		/**
		 * Final checksum of the previous simulation frame.
		 */
		static unsigned g_prevChecksum;

		/**
		 * @brief in synced code
		 *
		 * Whether one thread (doesn't have to current thread!!!) is currently processing a SimFrame.
		 */
		static int inSyncedCode;

#ifdef SYNC_HISTORY
		/**
		 * Sync hash logs
		 */
		static void LogHistory();

		static unsigned nextHistoryIndex;
		static unsigned nextFrameIndex;
		static std::array<unsigned, MAX_SYNC_HISTORY> logs;
		static std::array<unsigned, MAX_SYNC_HISTORY_FRAMES> logFrames;
#endif // SYNC_HISTORY
};

#endif // SYNCDEBUGGER_H
