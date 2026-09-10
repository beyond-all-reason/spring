/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#include "DemoFileExtension.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <ranges>

#include <zlib.h>

#include "demofile.h"
#include "System/StringUtil.h"

// Tools do not link the config subsystem, and accept any extension anyway.
#ifndef TOOLS
#include "System/Config/ConfigHandler.h"

CONFIG(std::string, DemoFileExtension).defaultValue("sdfz").description("Comma-separated list of replay file extensions. The first entry is used when recording; all entries are accepted when loading. Set by the lobby (e.g. 'barreplay,sdfz' for BAR).");

std::vector<std::string> GetDemoFileExtensions()
{
	const auto configValue = configHandler->GetString("DemoFileExtension");

	std::vector<std::string> extensions;

	for (const auto& part : configValue | std::views::split(',')) {
		auto extension = StringTrim(std::string(part.begin(), part.end()));

		if (!extension.empty() && extension.find_first_of("/\\.") == std::string::npos)
			extensions.emplace_back(std::move(extension));
	}

	if (extensions.empty())
		extensions.emplace_back("sdfz");
	return extensions;
}
#endif

bool IsDemoExtension(const std::string& ext)
{
#ifdef TOOLS
	return true;
#else
	std::string lower = ext;
	std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
	const auto extensions = GetDemoFileExtensions();
	return std::ranges::contains(extensions, lower);
#endif
}

bool ContentsLookLikeAReplay(const std::string& path)
{
	gzFile file = gzopen(path.c_str(), "rb");
	if (file == nullptr)
		return false;
	decltype(DemoFileHeader::magic) magic = {};
	const int bytesRead = gzread(file, magic, sizeof(magic));
	gzclose(file);
	return (bytesRead == static_cast<int>(sizeof(magic)) && memcmp(magic, DEMOFILE_MAGIC, sizeof(magic)) == 0);
}
