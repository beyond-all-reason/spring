/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Screenshot.h"

#include "Rendering/GL/myGL.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Textures/Bitmap.h"
#include "System/Config/ConfigHandler.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Log/ILog.h"
#include "System/StringUtil.h"
#include "System/Threading/ThreadPool.h"
#include "System/TimeUtil.h"

#include <vector>

#undef CreateDirectory

CONFIG(int, ScreenshotCounter)
    .description("Deprecated, does nothing, but not marked as such to keep compatibility with older engine versions")
    .defaultValue(0);

struct FunctionArgs {
	std::vector<uint8_t> pixelbuf;
	std::string filename;
	unsigned quality;
	int x;
	int y;
};

static std::shared_future<void> fut = {};

void TakeScreenshot(std::string type, unsigned quality)
{
	if (type.empty())
		type = "png";

	if (!FileSystem::CreateDirectory("screenshots"))
		return;

	if (fut.valid()) {
		fut.get();
		fut = {};
	}

	FunctionArgs args;
	args.x = globalRendering->winSizeX;
	args.y = globalRendering->winSizeY;
	args.x += ((4 - (args.x % 4)) * int((args.x % 4) != 0));

	// note: we no longer increment the counter until a "file not found" occurs
	// since that stalls the thread and might run concurrently with an IL write
	const std::string curTime = CTimeUtil::GetCurrentTimeStr(true);
	args.filename.assign("screenshots/screen_" + curTime + "." + type);
	args.quality = quality;
	args.pixelbuf.resize(args.x * args.y * 4);

	glReadPixels(0, 0, args.x, args.y, GL_RGBA, GL_UNSIGNED_BYTE, &args.pixelbuf[0]);

	fut = ThreadPool::Enqueue([](const FunctionArgs& args) {
		CBitmap bmp(&args.pixelbuf[0], args.x, args.y);
		bmp.ReverseYAxis();
		bmp.Save(args.filename, true, true, args.quality);
	}, args);
}
