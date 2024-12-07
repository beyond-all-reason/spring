/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _CFONTTEXTURE_H
#define _CFONTTEXTURE_H

#include <string>
#include <memory>

#include "Rendering/Textures/Bitmap.h"
#include "Rendering/Textures/IAtlasAllocator.h"
#include "Rendering/Textures/RowAtlasAlloc.h"
#include "System/UnorderedMap.hpp"
#include "System/UnorderedSet.hpp"
#include "System/Threading/WrappedSync.h"


struct FT_FaceRec_;
typedef struct FT_FaceRec_* FT_Face;
class CBitmap;

class FtLibraryHandlerProxy {
public:
	static void InitFtLibrary();
	static bool InitFontconfig(bool console);
};


struct IGlyphRect { //FIXME use SRect or float4
	IGlyphRect():
		x(0),y(0),
		w(0),h(0) {
	};

	IGlyphRect(float _x,float _y,float _w,float _h):
		x(_x),y(_y),
		w(_w),h(_h) {
	};

	float x0() const {
		return x;
	};
	float x1() const {
		return x+w;
	};
	float y0() const {
		return y;
	};
	float y1() const {
		return y+h;
	};

	float x,y;
	float w,h;
};


//wrapper to allow usage as shared_ptr
struct FontFileBytes {
	FontFileBytes(size_t size) {
		vec.resize(size);
	}
	using FT_Byte = unsigned char;
	FT_Byte* data();
private:
	std::vector<FT_Byte> vec;
};

//wrapper to allow usage as shared_ptr
struct FontFace {
	FontFace(FT_Face f, std::shared_ptr<FontFileBytes>& mem);
	~FontFace();
	operator FT_Face();

	FT_Face face;
	std::shared_ptr<FontFileBytes> memory;
};

struct GlyphInfo {
	GlyphInfo()
	: advance(0)
	, height(0)
	, descender(0)
	, index(0)
	, letter(0)
	, face(nullptr)
	{ };

	IGlyphRect size;
	IGlyphRect texCord;
	IGlyphRect shadowTexCord;
	float advance, height, descender;
	uint32_t index;
	char32_t letter;
	std::shared_ptr<FontFace> face;
};

class CglFontRenderer;

/**
This class just store glyphs and load new glyphs if requred
It works with image and don't care about rendering these glyphs
It works only and only with UTF32 chars
**/
class CFontTexture
{
public:
	friend class CglFontRenderer;
	friend class CglShaderFontRenderer;
	friend class CglNoShaderFontRenderer;
	friend class CglNullFontRenderer;

	static void InitFonts();
	static void KillFonts();
	static void Update();
	static bool AddFallbackFont(const std::string& fontfile);
	static void ClearFallbackFonts();
	static void ClearAllGlyphs();

	inline static spring::WrappedSyncRecursiveMutex sync = {};
protected:
	CFontTexture(const std::string& fontfile, int size, int outlinesize, float  outlineweight);
	virtual ~CFontTexture();
public:
	int GetSize() const { return fontSize; }
	int GetTextureWidth() const { return texWidth; }
	int GetTextureHeight() const { return texHeight; }
	int   GetOutlineWidth() const { return outlineSize; }
	float GetOutlineWeight() const { return outlineWeight; }
	float GetLineHeight() const { return lineHeight; }
	float GetDescender() const { return fontDescender; }
	int GetTexture() const { return glyphAtlasTextureID; }

	const std::string& GetFamily() const { return fontFamily; }
	const std::string& GetStyle() const { return fontStyle; }

	const GlyphInfo& GetGlyph(char32_t ch); //< Get a glyph
public:
	void ReallocAtlases(bool pre);
	bool HasColor() const { return needsColor; }
protected:
	void LoadWantedGlyphs(char32_t begin, char32_t end);
	void LoadWantedGlyphs(const std::vector<char32_t>& wanted);
	bool GlyphAtlasTextureNeedsUpdate() const;
	bool GlyphAtlasTextureNeedsUpload() const;
	void UpdateGlyphAtlasTexture();
	void UploadGlyphAtlasTexture();
	void UploadGlyphAtlasTextureImpl();
private:
	void CreateTexture(const int width, const int height, const bool init);
	void LoadGlyph(std::shared_ptr<FontFace>& f, char32_t ch, unsigned index);
	bool ClearGlyphs();
	void PreloadGlyphs();
protected:
	float GetKerning(const GlyphInfo& lgl, const GlyphInfo& rgl);
protected:
	static inline std::vector<std::weak_ptr<CFontTexture>> allFonts = {};

	static inline const GlyphInfo dummyGlyph = GlyphInfo();
	static inline bool needsClearGlyphs = false;

	std::array<float, 128 * 128> kerningPrecached = {}; // contains ASCII kerning

	int outlineSize;
	float outlineWeight;
	float lineHeight;
	float fontDescender;
	float normScale;
	int fontSize;
	std::string fontFamily;
	std::string fontStyle;

	int texWidth;
	int texHeight;
	int wantedTexWidth;
	int wantedTexHeight;
	bool needsColor;
	bool isColor;

	unsigned int glyphAtlasTextureID = 0;

	inline static bool needThreadSafety = true;

	std::unique_ptr<CglFontRenderer> fontRenderer;
private:
#ifndef HEADLESS
	int curTextureUpdate = 0;
	int lastTextureUpdate = 0;
	bool needsTextureUpload = true;
	inline static int maxFontTries = 0;
#endif
	std::shared_ptr<FontFace> shFace;

	spring::unordered_map<char32_t, int> failedAttemptsToReplace;
	spring::unordered_map<char32_t, GlyphInfo> glyphs; // UTF32 -> GlyphInfo
	spring::unordered_map<uint64_t, float> kerningDynamic; // contains unicode kerning

	std::vector<CBitmap> atlasGlyphs;

	CRowAtlasAlloc atlasAlloc;

	CBitmap atlasUpdate;
	CBitmap atlasUpdateShadow;

	static std::vector<char32_t> nonPrintableRanges;
public:
	auto GetGlyphs() const -> const decltype(glyphs) { return glyphs; }
	auto GetGlyphs()       ->       decltype(glyphs) { return glyphs; }
};

#endif // CFONTTEXTURE_H
