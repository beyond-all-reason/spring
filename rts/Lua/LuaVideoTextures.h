/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <cstddef>
#include <cstdint>
#include <array>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>

#include "Rendering/GL/myGL.h"
#include "Rendering/GL/PBO.h"
#include "Rendering/Video/VideoPlayer.h"

class LuaVideoTextures {
public:
	static constexpr char prefix = '@';

	LuaVideoTextures() = default;
	~LuaVideoTextures() { FreeAll(); }

	LuaVideoTextures(const LuaVideoTextures&) = delete;
	LuaVideoTextures& operator=(const LuaVideoTextures&) = delete;

	std::string Create(const std::string& path, const std::string& vfsModes, const video::VideoOptions& options, std::string& error);
	bool Free(const std::string& name);
	void FreeAll();
	void Clear() { FreeAll(); }

	bool Play(const std::string& name);
	bool Pause(const std::string& name);
	bool Stop(const std::string& name);
	bool Seek(const std::string& name, double seconds);
	bool SetVolume(const std::string& name, float volume);

	video::VideoInfo GetInfo(const std::string& name) const;
	bool Exists(const std::string& name) const;
	std::uint64_t GetHandle(const std::string& name) const;
	GLuint GetTextureID(std::uint64_t handle);
	std::tuple<int, int, int> GetSize(std::uint64_t handle) const;

private:
	struct Texture {
		std::unique_ptr<video::VideoPlayer> player;
		GLuint id = 0;
		int width = 0;
		int height = 0;
		unsigned int lastUploadFrame = 0;
		std::array<std::unique_ptr<PBO>, 2> uploadBuffers;
		std::size_t nextUploadBuffer = 0;
	};

	Texture* Get(std::uint64_t handle);
	const Texture* Get(std::uint64_t handle) const;

	std::unordered_map<std::uint64_t, Texture> textures;
};
