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
	, depthTexture{ 0 }
	, depthFBO{ nullptr }
{
	eventHandler.AddClient(this);
	ViewResize();
}

DepthBufferCopy::~DepthBufferCopy()
{
	if (depthFBO) {
		if (depthFBO->IsValid()) {
			depthFBO->Bind();
			depthFBO->DetachAll();
			depthFBO->Unbind();
		}
		depthFBO->Kill();
		depthFBO = nullptr;
	}

	if (depthTexture > 0u) {
		glDeleteTextures(1, &depthTexture);
		depthTexture = 0u;
	}

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

void DepthBufferCopy::ViewResize()
{
	if (depthTexture != 0u) {
		glDeleteTextures(1, &depthTexture);
		depthTexture = 0u;
	}

	glGenTextures(1, &depthTexture);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, depthTexture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

	GLint depthFormat = static_cast<GLint>(CGlobalRendering::DepthBitsToFormat(globalRendering->supportDepthBufferBitDepth));
	glTexImage2D(GL_TEXTURE_2D, 0, depthFormat, globalRendering->viewSizeX, globalRendering->viewSizeY, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

	glBindTexture(GL_TEXTURE_2D, 0);

	if (depthFBO) {
		if (depthFBO->IsValid()) {
			depthFBO->Bind();
			depthFBO->DetachAll();
			depthFBO->Unbind();
		}
		depthFBO->Kill();
		depthFBO = nullptr; //probably redundant
	}

	depthFBO = std::make_unique<FBO>(true); //probably redundant
	depthFBO->Init(false);

	depthFBO->Bind();
	depthFBO->AttachTexture(depthTexture, GL_TEXTURE_2D, GL_DEPTH_ATTACHMENT);
	glDrawBuffer(GL_NONE);
	depthFBO->CheckStatus("DEPTH-BUFFER-COPY-FBO");
	depthFBO->Unbind();
}

bool DepthBufferCopy::IsValid() const { return depthFBO && depthFBO->IsValid() && depthTexture > 0; }

void DepthBufferCopy::MakeDepthBufferCopy() const
{
	const std::array<int, 4> srcScreenRect = { globalRendering->viewPosX, globalRendering->viewPosY, globalRendering->viewPosX + globalRendering->viewSizeX, globalRendering->viewPosY + globalRendering->viewSizeY };
	const std::array<int, 4> dstScreenRect = { 0, 0, globalRendering->viewSizeX, globalRendering->viewSizeY };

	FBO::Blit(-1, depthFBO->GetId(), srcScreenRect, dstScreenRect, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
}
