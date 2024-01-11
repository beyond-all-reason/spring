#include "DepthBufferCopy.h"

#include <array>
#include <cassert>

#include "System/EventHandler.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/FBO.h"

std::unique_ptr<DepthBufferCopy> depthBufferCopy = nullptr;

DepthBufferCopy::DepthBufferCopy()
	: CEventClient("[DepthBufferCopy]", 012345, false)
{
	eventHandler.AddClient(this);
	ViewResize();
}

DepthBufferCopy::~DepthBufferCopy()
{
	assert(references[false].empty());
	assert(references[true ].empty());

	eventHandler.RemoveClient(this);
	autoLinkedEvents.clear();
}

void DepthBufferCopy::Init()
{
	depthBufferCopy = std::make_unique<DepthBufferCopy>();
}

void DepthBufferCopy::Kill()
{
	depthBufferCopy = nullptr;
}

void DepthBufferCopy::AddConsumer(bool ms, const void* p)
{
	if (references[ms].empty())
		CreateTextureAndFBO(ms);

	references[ms].emplace(reinterpret_cast<uintptr_t>(p));
}

void DepthBufferCopy::DelConsumer(bool ms, const void* p)
{
	if (auto it = references[ms].find(reinterpret_cast<uintptr_t>(p)); it != references[ms].end())
		references[ms].erase(it);

	if (!references[ms].empty())
		return;

	DestroyTextureAndFBO(ms);
}

void DepthBufferCopy::ViewResize()
{
	for (size_t ms = 0; ms < depthFBOs.size(); ++ms) {
		if (references[ms].empty())
			continue;

		RecreateTextureAndFBO(static_cast<bool>(ms));
	}
}

bool DepthBufferCopy::IsValid(bool ms) const {
	const auto& depthFBO = depthFBOs[ms];
	return depthFBO && depthFBO->IsValid() && depthTextures[ms] > 0;
}

void DepthBufferCopy::MakeDepthBufferCopy() const
{
	const std::array<int, 4> srcScreenRect = { globalRendering->viewPosX, globalRendering->viewPosY, globalRendering->viewPosX + globalRendering->viewSizeX, globalRendering->viewPosY + globalRendering->viewSizeY };
	const std::array<int, 4> dstScreenRect = { 0, 0, globalRendering->viewSizeX, globalRendering->viewSizeY };

	/*
	for (size_t ms = 0; ms < depthFBOs.size(); ++ms) {
		if (references[ms].empty())
			continue;

		FBO::Blit(-1, depthFBOs[ms]->GetId(), srcScreenRect, dstScreenRect, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	}
	*/
	if (!references[true ].empty())
		FBO::Blit(      -1, depthFBOs[true ]->GetId(), srcScreenRect, dstScreenRect, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

	if (!references[false].empty()) {
		const auto srcFboID = depthFBOs[true] ? depthFBOs[true]->GetId() : -1;
		FBO::Blit(srcFboID, depthFBOs[false]->GetId(), srcScreenRect, dstScreenRect, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	}
}

void DepthBufferCopy::DestroyTextureAndFBO(bool ms)
{
	auto& depthFBO = depthFBOs[ms];
	auto& depthTexture = depthTextures[ms];

	assert(depthFBO);
	if (depthFBO) {
		if (depthFBO->IsValid()) {
			depthFBO->Bind();
			depthFBO->DetachAll();
			depthFBO->Unbind();
		}
		depthFBO->Kill();
		depthFBO = nullptr;
	}

	assert(depthTexture);
	if (depthTexture) {
		glDeleteTextures(1, &depthTexture);
		depthTexture = 0u;
	}
}

void DepthBufferCopy::CreateTextureAndFBO(bool ms)
{
	auto& depthTexture = depthTextures[ms];
	auto& depthFBO     = depthFBOs[ms];
	const auto target = ms ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;

	assert(depthTexture == 0);
	glGenTextures(1, &depthTexture);

	glEnable(target);
	glBindTexture(target, depthTexture);

	glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(target, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
	glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, 0);
	glTexParameteri(target, GL_TEXTURE_COMPARE_MODE, GL_NONE);

	GLint depthFormat = static_cast<GLint>(CGlobalRendering::DepthBitsToFormat(globalRendering->supportDepthBufferBitDepth));
	if (target == GL_TEXTURE_2D)
		glTexImage2D(target, 0, depthFormat, globalRendering->viewSizeX, globalRendering->viewSizeY, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	else
		glTexImage2DMultisample(target, globalRendering->msaaLevel, depthFormat, globalRendering->viewSizeX, globalRendering->viewSizeY, GL_TRUE);

	glBindTexture(target, 0);
	glDisable(target);

	assert(depthFBO == nullptr);
	depthFBO = std::make_unique<FBO>(true);
	depthFBO->Init(false);

	depthFBO->Bind();
	depthFBO->AttachTexture(depthTexture, target, GL_DEPTH_ATTACHMENT);
	glDrawBuffer(GL_NONE);
	depthFBO->CheckStatus("DEPTH-BUFFER-COPY-FBO" + ms ? "-MULTISAMPLED" : "");
	depthFBO->Unbind();
}
