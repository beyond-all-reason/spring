/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SYNCCHECKER_H
#define SYNCCHECKER_H

#ifdef SYNCCHECK

#include "System/SpringHash.h"

#include <assert.h>

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
		static void NewFrame() { g_checksum = 0xfade1eaf; }
		static void debugSyncCheckThreading();
		static void Sync(const void* p, unsigned size) {
#ifdef DEBUG_SYNC_MT_CHECK
			// Sync calls should not be occurring in multi-threaded sections
			debugSyncCheckThreading();
#endif
			// most common cases first, make it easy for compiler to optimize for it
			// simple xor is not enough to detect multiple zeroes, e.g.
			g_checksum = spring::LiteHash(p, size, g_checksum);
			//LOG("[Sync::Checker] chksum=%u\n", g_checksum);
		}

	private:

		/**
		 * The sync checksum
		 */
		static unsigned g_checksum;

		/**
		 * @brief in synced code
		 *
		 * Whether one thread (doesn't have to current thread!!!) is currently processing a SimFrame.
		 */
		static int inSyncedCode;
};

#endif // SYNCDEBUG

#endif // SYNCDEBUGGER_H
