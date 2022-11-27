#pragma once

#include <memory>

#include "Rendering/GL/VertexArrayTypes.h"
#include "Rendering/GL/RenderBuffers.h"

class CglFont;
class CFontTexture;
class CglFontRenderer {
public:
	virtual ~CglFontRenderer() = default;

	virtual void AddQuadTrianglesPB(VA_TYPE_TC&& tl, VA_TYPE_TC&& tr, VA_TYPE_TC&& br, VA_TYPE_TC&& bl) = 0;
	virtual void AddQuadTrianglesOB(VA_TYPE_TC&& tl, VA_TYPE_TC&& tr, VA_TYPE_TC&& br, VA_TYPE_TC&& bl) = 0;
	virtual void DrawTraingleElements() = 0;
	virtual void HandleTextureUpdate(CFontTexture& font, bool onlyUpload) = 0;
	virtual void PushGLState(const CglFont& font) = 0;
	virtual void PopGLState() = 0;

	virtual bool IsLegacy() const = 0;
	virtual bool IsValid() const = 0;
	virtual void GetStats(std::array<size_t, 8>& stats) const = 0;

	static std::unique_ptr<CglFontRenderer> CreateInstance();
	static void DeleteInstance(std::unique_ptr<CglFontRenderer>& instance);
protected:
	GLint currProgID = 0;

	// should be enough to hold all data for a given frame
	static constexpr size_t NUM_BUFFER_ELEMS = (1 << 14);
	static constexpr size_t NUM_TRI_BUFFER_VERTS = (4 * NUM_BUFFER_ELEMS);
	static constexpr size_t NUM_TRI_BUFFER_ELEMS = (6 * NUM_BUFFER_ELEMS);
};

namespace Shader {
	struct IProgramObject;
};
class CglShaderFontRenderer final: public CglFontRenderer {
public:
	CglShaderFontRenderer();
	~CglShaderFontRenderer() override;

	void AddQuadTrianglesPB(VA_TYPE_TC&& tl, VA_TYPE_TC&& tr, VA_TYPE_TC&& br, VA_TYPE_TC&& bl) override;
	void AddQuadTrianglesOB(VA_TYPE_TC&& tl, VA_TYPE_TC&& tr, VA_TYPE_TC&& br, VA_TYPE_TC&& bl) override;
	void DrawTraingleElements() override;
	void HandleTextureUpdate(CFontTexture& font, bool onlyUpload) override;
	void PushGLState(const CglFont& font) override;
	void PopGLState() override;

	bool IsLegacy() const override { return false; }
	bool IsValid() const override { return fontShader->IsValid(); }
	void GetStats(std::array<size_t, 8>& stats) const override;
private:
	TypedRenderBuffer<VA_TYPE_TC> primaryBufferTC;
	TypedRenderBuffer<VA_TYPE_TC> outlineBufferTC;

	static inline size_t fontShaderRefs = 0;
	static inline std::unique_ptr<Shader::IProgramObject> fontShader = nullptr;
};

class CglNoShaderFontRenderer final: public CglFontRenderer {
public:
	CglNoShaderFontRenderer();
	~CglNoShaderFontRenderer() override;

	void AddQuadTrianglesPB(VA_TYPE_TC&& tl, VA_TYPE_TC&& tr, VA_TYPE_TC&& br, VA_TYPE_TC&& bl) override;
	void AddQuadTrianglesOB(VA_TYPE_TC&& tl, VA_TYPE_TC&& tr, VA_TYPE_TC&& br, VA_TYPE_TC&& bl) override;
	void DrawTraingleElements() override;
	void HandleTextureUpdate(CFontTexture& font, bool onlyUpload) override;
	void PushGLState(const CglFont& font) override;
	void PopGLState() override;

	bool IsLegacy() const override { return true; }
	bool IsValid() const override { return true; }
	void GetStats(std::array<size_t, 8>& stats) const override;
private:
	void AddQuadTrianglesImpl(bool primary, VA_TYPE_TC&& tl, VA_TYPE_TC&& tr, VA_TYPE_TC&& br, VA_TYPE_TC&& bl);

	std::array<std::vector<VA_TYPE_TC>, 2> verts; // OL, PM
	std::array<std::vector<uint16_t  >, 2> indcs; // OL, PM

	uint32_t textureSpaceMatrix = 0u;
};

class CglNullFontRenderer final : public CglFontRenderer {
	// Inherited via CglFontRenderer
	void AddQuadTrianglesPB(VA_TYPE_TC&& tl, VA_TYPE_TC&& tr, VA_TYPE_TC&& br, VA_TYPE_TC&& bl) override {}
	void AddQuadTrianglesOB(VA_TYPE_TC&& tl, VA_TYPE_TC&& tr, VA_TYPE_TC&& br, VA_TYPE_TC&& bl) override {}
	void DrawTraingleElements() override {}
	void HandleTextureUpdate(CFontTexture& font, bool onlyUpload) override {}
	void PushGLState(const CglFont& font) override {}
	void PopGLState() override {}
	bool IsLegacy() const override { return true; }
	bool IsValid() const override { return true; }
	void GetStats(std::array<size_t, 8>& stats) const override;
};