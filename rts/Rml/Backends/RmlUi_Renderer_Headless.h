#ifndef RMLUI_RENDERER_HEADLESS_H
#define RMLUI_RENDERER_HEADLESS_H

#include <RmlUi/Core/RenderInterface.h>
#include <RmlUi/Core/Types.h>

class RenderInterface_Headless : public Rml::RenderInterface
{
public:
	RenderInterface_Headless(){};
	~RenderInterface_Headless(){};
	explicit operator bool() const { return true; }
	void RenderGeometry(Rml::Vertex *vertices, int num_vertices, int *indices, int num_indices, Rml::TextureHandle texture, const Rml::Vector2f &translation) {};

	Rml::CompiledGeometryHandle CompileGeometry(Rml::Vertex *vertices, int num_vertices, int *indices, int num_indices, Rml::TextureHandle texture)
	{
		return (Rml::CompiledGeometryHandle)nullptr;
	};
	void RenderCompiledGeometry(Rml::CompiledGeometryHandle geometry, const Rml::Vector2f &translation){};
	void ReleaseCompiledGeometry(Rml::CompiledGeometryHandle geometry){};
	void SetScissorRegion(int x, int y, int width, int height) {};
	void EnableScissorRegion(bool enable) {};
	bool LoadTexture(Rml::TextureHandle &texture_handle, Rml::Vector2i &texture_dimensions, const Rml::String &source)
	{
		return true;
	};
	bool GenerateTexture(Rml::TextureHandle &texture_handle, const Rml::byte *source, const Rml::Vector2i &source_dimensions)
	{
		return true;
	};
	void ReleaseTexture(Rml::TextureHandle texture){};
	void SetTransform(const Rml::Matrix4f *transform){};
	void BeginFrame(){};
	void EndFrame(){};
	void SetViewport(int x, int y){};
};

#endif // RMLUI_RENDERER_HEADLESS_H
