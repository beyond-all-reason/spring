/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifdef TRACY_ENABLE

#include "Backend.h"
#include "LogUtil.h"
#include "lib/fmt/format.h"

#include <algorithm>
#include "System/Misc/TracyDefs.h"

static void log_sink_record_tracy(int level, const char* section, const char* record)
{
	auto buf = fmt::format("[{}:{}][{}] {}", log_util_levelToString(log_util_getNearestLevel(level)),
	                       level, section, record);
	TracyMessageS(buf.c_str(), buf.size(), 30);
}

namespace {

/// Auto-registers the sink defined in this file before main() is called
struct TracySinkRegistrator {
	TracySinkRegistrator() {
		log_backend_registerSink(&log_sink_record_tracy);
	}
	~TracySinkRegistrator() {
		log_backend_unregisterSink(&log_sink_record_tracy);
	}
} tracySinkRegistrator;

}

#endif
