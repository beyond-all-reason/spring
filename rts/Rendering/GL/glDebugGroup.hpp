#pragma once

#include <memory>
#include <cstdint>
#include "System/StringHash.h"
#include "System/StringUtil.h"

namespace GL {
	class DebugGroup {
	public:
		virtual ~DebugGroup() {};
		static std::unique_ptr<DebugGroup> GetScoped(uint32_t id, const char* messsage);
	};
	class DebugGroupNoop : public DebugGroup {
	public:
		DebugGroupNoop(uint32_t id, const char* messsage) {}
	};
	class DebugGroupImpl : public DebugGroup {
	public:
		DebugGroupImpl(uint32_t id, const char* messsage);
		~DebugGroupImpl() override final;
	};
}

#define SCOPED_GL_DEBUGGROUP(name) const auto _UTIL_CONCAT(__scopedGLDebugGroup, __LINE__) = GL::DebugGroup::GetScoped(0x824A/*GL_DEBUG_SOURCE_APPLICATION*/ ,name)