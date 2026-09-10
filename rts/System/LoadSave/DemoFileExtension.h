/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <string>
#include <vector>

std::vector<std::string> GetDemoFileExtensions();
bool IsDemoExtension(const std::string& ext);
bool ContentsLookLikeAReplay(const std::string& path);
