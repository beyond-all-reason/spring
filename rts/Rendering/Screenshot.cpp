/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Screenshot.h"

#include <vector>

#include "Rendering/GL/myGL.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Textures/Bitmap.h"
#include "System/StringUtil.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Threading/ThreadPool.h"
#include "System/TimeUtil.h"

#undef CreateDirectory

CONFIG(int, ScreenshotCounter).description("Deprecated, does nothing, but not marked as such to keep compatibility with older engine versions").defaultValue(0);

struct FunctionArgs
{
	std::vector<uint8_t> pixelbuf;
	std::vector<float> floatPixelbuf;
	std::string filename;
	unsigned quality;
	int x;
	int y;
	bool hdr = false;
};

static std::shared_future<void> fut = {};

void TakeScreenshot(std::string type, unsigned quality)
{
	if (type.empty())
		type = "png";
	if (StringToLower(type) == "hdr") {
		TakeHDRScreenshot();
		return;
	}

	if (!FileSystem::CreateDirectory("screenshots"))
		return;

	if (fut.valid()) {
		fut.get();
		fut = {};
	}

	FunctionArgs args;
	args.x  = globalRendering->winSizeX;
	args.y  = globalRendering->winSizeY;

	// note: we no longer increment the counter until a "file not found" occurs
	// since that stalls the thread and might run concurrently with an IL write
	const std::string curTime = CTimeUtil::GetCurrentTimeStr(true);
	args.filename.assign("screenshots/screen_" + curTime + "." + type);
	args.quality = quality;
	args.pixelbuf.resize(args.x * args.y * 4);

	GLint oldReadFramebuffer = 0;
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &oldReadFramebuffer);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glReadPixels(0, 0, args.x, args.y, GL_RGBA, GL_UNSIGNED_BYTE, args.pixelbuf.data());
	glBindFramebuffer(GL_READ_FRAMEBUFFER, oldReadFramebuffer);

	fut = ThreadPool::Enqueue([](const FunctionArgs& args) {
		CBitmap bmp(args.pixelbuf.data(), args.x, args.y);
		bmp.ReverseYAxis();
		bmp.Save(args.filename, true, true, args.quality);
	}, args);
}

void TakeHDRScreenshot()
{
	if (!globalRendering->IsSceneTargetActive() || globalRendering->GetSceneColorTexture() == 0) {
		LOG_L(L_WARNING, "[TakeHDRScreenshot] no scene-linear HDR render target is available");
		return;
	}
	if (!FileSystem::CreateDirectory("screenshots"))
		return;
	if (fut.valid()) {
		fut.get();
		fut = {};
	}

	FunctionArgs args;
	args.x = globalRendering->winSizeX;
	args.y = globalRendering->winSizeY;
	args.hdr = true;
	args.quality = 100;
	args.filename = "screenshots/hdr_scene_" + CTimeUtil::GetCurrentTimeStr(true) + ".hdr";
	args.floatPixelbuf.resize(std::size_t(args.x) * args.y * 3);

	GLint oldTexture = 0;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTexture);
	glBindTexture(GL_TEXTURE_2D, globalRendering->GetSceneColorTexture());
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, args.floatPixelbuf.data());
	glBindTexture(GL_TEXTURE_2D, oldTexture);

	fut = ThreadPool::Enqueue([](const FunctionArgs& args) {
		CBitmap bmp(reinterpret_cast<const uint8_t*>(args.floatPixelbuf.data()), args.x, args.y, 3, GL_FLOAT);
		bmp.ReverseYAxis();
		if (!bmp.Save(args.filename, true, true, args.quality))
			LOG_L(L_ERROR, "[TakeHDRScreenshot] failed to save \"%s\"", args.filename.c_str());
	}, args);
}
