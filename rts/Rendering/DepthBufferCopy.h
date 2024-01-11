#pragma once

#include <memory>
#include <array>
#include <cstdint>

#include "System/UnorderedSet.hpp"
#include "System/EventClient.h"

class FBO;

class DepthBufferCopy : public CEventClient {
public:
	DepthBufferCopy();
	~DepthBufferCopy() override;

	static void Init();
	static void Kill();

	void AddConsumer(bool ms, const void* p);
	void DelConsumer(bool ms, const void* p);

	bool WantsEvent(const std::string& eventName) override {
		return
			(eventName == "ViewResize");
	}

	void ViewResize() override;

	bool IsValid(bool ms) const;

	void MakeDepthBufferCopy() const;
	uint32_t GetDepthBufferTexture(bool ms) const { return depthTextures[ms]; }
private:
	void DestroyTextureAndFBO(bool ms);
	void CreateTextureAndFBO(bool ms);
	void RecreateTextureAndFBO(bool ms) {
		DestroyTextureAndFBO(ms);
		CreateTextureAndFBO(ms);
	}

	std::array<spring::unordered_set<uintptr_t>, 2> references = {};
	std::array<uint32_t, 2> depthTextures = {};
	std::array<std::unique_ptr<FBO>, 2> depthFBOs = {};
};

extern std::unique_ptr<DepthBufferCopy> depthBufferCopy;