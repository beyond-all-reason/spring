/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <array>
#include <string_view>

#include "IAudioChannel.h"

enum ChannelType : size_t {
	CHANNEL_BGMUSIC   = 0,
	CHANNEL_GENERAL   = 1,
	CHANNEL_BATTLE    = 2,
	CHANNEL_UNITREPLY = 3,
	CHANNEL_UI        = 4,
	CHANNEL_AMBIENT   = 5,
	CHANNEL_COUNT     = 6
};

using namespace std::literals;
static constexpr std::array ChannelNames {
	"Music"sv,
	"General"sv,
	"Battle"sv,
	"UnitReply"sv,
	"UserInterface"sv,
	"Ambient"sv
};

extern std::array<IAudioChannel*, ChannelType::CHANNEL_COUNT> Channels;