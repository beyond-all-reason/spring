/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef RMLUI_RENDERER_HEADLESS_H
#define RMLUI_RENDERER_HEADLESS_H

#include <RmlUi/Core/RenderInterface.h>
#include <RmlUi/Core/Types.h>

class RenderInterface_Headless : public Rml::RenderInterface
{
public:
	RenderInterface_Headless() = default;
	~RenderInterface_Headless() override = default;
	explicit operator bool() const { return true; }
	Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex>, Rml::Span<const int>) override
	{
		return (Rml::CompiledGeometryHandle) nullptr;
	}
	void RenderGeometry(Rml::CompiledGeometryHandle, Rml::Vector2f, Rml::TextureHandle) override {}
	void ReleaseGeometry(Rml::CompiledGeometryHandle) override {}

	Rml::TextureHandle LoadTexture(Rml::Vector2i&, const Rml::String&) override
	{
		return (Rml::TextureHandle) nullptr;
	}
	Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte>, Rml::Vector2i) override
	{
		return (Rml::TextureHandle) nullptr;
	}
	void ReleaseTexture(Rml::TextureHandle) override {}

	void EnableScissorRegion(bool) override {}
	void SetScissorRegion(Rml::Rectanglei) override {}

	void SetViewport(int, int) {}
	void BeginFrame() {}
	void EndFrame() {}
};

#endif // RMLUI_RENDERER_HEADLESS_H
