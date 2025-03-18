#pragma once

#include <memory>
#include <cstdint>
#include "System/StringHash.h"

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

#define SCOPED_GL_DEBUGGROUP(name) const auto __scopedGLDebugGroup = GL::DebugGroup::GetScoped(hashString(name) ,name)