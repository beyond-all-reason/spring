#include "glDebugGroup.hpp"

#include "myGL.h"

GL::DebugGroupImpl::DebugGroupImpl(uint32_t id, const char* messsage)
{
	glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, id, -1, messsage);
}

GL::DebugGroupImpl::~DebugGroupImpl()
{
	glPopDebugGroup();
}

std::unique_ptr<GL::DebugGroup> GL::DebugGroup::GetScoped(uint32_t id, const char* messsage)
{
	if (GLAD_GL_KHR_debug)
		return std::make_unique<GL::DebugGroupImpl>(id, messsage);
	else
		return std::make_unique<GL::DebugGroupNoop>(id, messsage);
}
