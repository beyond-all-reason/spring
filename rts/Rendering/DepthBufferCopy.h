#pragma once

#include <memory>
#include <cstdint>

#include "System/EventClient.h"

class FBO;

class DepthBufferCopy : public CEventClient {
public:
	DepthBufferCopy();
	~DepthBufferCopy() override;

	static void Init();
	static void Kill();

	bool WantsEvent(const std::string& eventName) override {
		return
			(eventName == "ViewResize");
	}

	void ViewResize() override;

	bool IsValid() const;

	void MakeDepthBufferCopy() const;
	uint32_t GetDepthBufferTexture() const { return depthTexture; }
private:
	uint32_t depthTexture = 0u;
	std::unique_ptr<FBO> depthFBO = nullptr;
};

extern std::unique_ptr<DepthBufferCopy> depthBufferCopy;