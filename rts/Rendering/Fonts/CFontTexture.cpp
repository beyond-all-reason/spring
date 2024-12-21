/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CFontTexture.h"
#include "glFontRenderer.h"
#include "FontLogSection.h"

#include <cstring> // for memset, memcpy
#include <string>
#include <vector>
#include <sstream>

#ifndef HEADLESS
	#include <ft2build.h>
	#include FT_FREETYPE_H
	#ifdef USE_FONTCONFIG
		#include <fontconfig/fontconfig.h>
		#include <fontconfig/fcfreetype.h>
	#endif
#endif // HEADLESS

#include "Rendering/GL/myGL.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Textures/Bitmap.h"
#include "System/Config/ConfigHandler.h"
#include "System/Exceptions.h"
#include "System/EventHandler.h"
#include "System/Log/ILog.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Threading/ThreadPool.h"
#ifdef _DEBUG
	#include "System/Platform/Threading.h"
#endif
#include "System/SafeUtil.h"
#include "System/StringUtil.h"
#include "System/TimeProfiler.h"
#include "System/UnorderedMap.hpp"
#include "System/float4.h"
#include "System/ContainerUtil.h"
#include "System/ScopedResource.h"
#include "fmt/format.h"
#include "fmt/printf.h"

#include "System/Misc/TracyDefs.h"

#define SUPPORT_AMD_HACKS_HERE

#ifndef HEADLESS
	#undef __FTERRORS_H__
	#define FT_ERRORDEF( e, v, s )  { e, s },
	#define FT_ERROR_START_LIST     {
	#define FT_ERROR_END_LIST       { 0, 0 } };
	struct FTErrorRecord {
		int          err_code;
		const char*  err_msg;
	} static errorTable[] =
	#include FT_ERRORS_H

	struct IgnoreMe {}; // MSVC IntelliSense is confused by #include FT_ERRORS_H above. This seems to fix it.

	static const char* GetFTError(FT_Error e) {
		const auto it = std::find_if(std::begin(errorTable), std::end(errorTable), [e](FTErrorRecord er) { return er.err_code == e; });
		if (it != std::end(errorTable))
			return it->err_msg;

		return "Unknown error";
	}
#endif // HEADLESS





#ifdef HEADLESS
typedef unsigned char FT_Byte;
#endif


static spring::unordered_map<std::string, std::weak_ptr<FontFace>> fontFaceCache;
static spring::unordered_map<std::string, std::weak_ptr<FontFileBytes>> fontMemCache;
static spring::unordered_set<std::pair<std::string, int>, spring::synced_hash<std::pair<std::string, int>>> invalidFonts;
static auto cacheMutexes = spring::WrappedSyncRecursiveMutex{};

#include "NonPrintableSymbols.inl"


#ifndef HEADLESS
class FtLibraryHandler {
public:
	FtLibraryHandler()
		: config(nullptr)
		, lib(nullptr)
		#ifdef USE_FONTCONFIG
		, gameFontSet(nullptr)
		, basePattern(nullptr)
		#endif // USE_FONTCONFIG
	{
		const FT_Error error = FT_Init_FreeType(&lib);

		if (error != 0) {
			FT_Int version[3];
			FT_Library_Version(lib, &version[0], &version[1], &version[2]);

			std::string err = fmt::sprintf("[%s] FT_Init_FreeType failure (version %d.%d.%d) \"%s\"",
						       __func__, version[0], version[1], version[2], GetFTError(error));
			throw std::runtime_error(err);
		}
	}

	~FtLibraryHandler() {
		FT_Done_FreeType(lib);

		#ifdef USE_FONTCONFIG
		if (!config)
			return;

		FcConfigDestroy(config);
		if (gameFontSet) {
			FcFontSetDestroy(gameFontSet);
		}
		if (basePattern) {
			FcPatternDestroy(basePattern);
		}
		FcFini();
		config = nullptr;
		#endif
	}

	bool InitFontconfig(bool console) {
		#ifdef USE_FONTCONFIG
		auto LOG_MSG = [console](const std::string& fmt, bool isError, auto&&... args) {
			if (console) {
				std::string fmtNL = fmt + "\n";
				printf(fmtNL.c_str(), args...);
			}
			else {
				if (isError) {
					LOG_L(L_ERROR, fmt.c_str(), args...);
				}
				else {
					LOG(fmt.c_str(), args...);
				}
			}
		};

		if (!UseFontConfig())
			return false;

		{
			std::string msg = fmt::sprintf("%s::FontConfigInit (version %d.%d.%d)", __func__, FC_MAJOR, FC_MINOR, FC_REVISION);
			ScopedOnceTimer timer(msg);
			ZoneScopedNC("FtLibraryHandler::FontConfigInit", tracy::Color::Purple);

			FcBool res;
			std::string errprefix = fmt::sprintf("[%s] Fontconfig(version %d.%d.%d) failed to initialize", __func__, FC_MAJOR, FC_MINOR, FC_REVISION);

			// init configuration
			FcConfigEnableHome(FcFalse);
			config = FcConfigCreate();

			// we cant directly use the usual fontconfig methods because those won't let us have both first our cache
			// and system fonts included. also linux actually has system config files that can be used by fontconfig.

			#ifdef _WIN32
			static constexpr auto winFontPath = "%WINDIR%\\fonts";
			const int neededSize = ExpandEnvironmentStrings(winFontPath, nullptr, 0);
			std::vector <char> osFontsDir (neededSize);
			ExpandEnvironmentStrings(winFontPath, osFontsDir.data(), osFontsDir.size());

			static constexpr const char* configFmt = R"(<fontconfig><dir>%s</dir><cachedir>fontcache</cachedir></fontconfig>)";
			const std::string configFmtVar = fmt::sprintf(configFmt, osFontsDir.data());
			#else
			const std::string configFmtVar = R"(<fontconfig><cachedir>fontcache</cachedir></fontconfig>)";
			#endif

			#ifdef _WIN32
			// Explicitly set the config with xml for windows.
			res = FcConfigParseAndLoadFromMemory(config, reinterpret_cast<const FcChar8*>(configFmtVar.c_str()), FcTrue);
			#else
			// Load system configuration (passing 0 here so fc will use the default os config file if possible).
			res = FcConfigParseAndLoad(config, 0, true);
			#endif
			if (res) {
				#ifndef _WIN32
				// add local cache after system config for linux
				FcConfigParseAndLoadFromMemory(config, reinterpret_cast<const FcChar8*>(configFmtVar.c_str()), FcTrue);
				#endif

				LOG_MSG("[%s] Using Fontconfig light init", false, __func__);

				// build system fonts
				res = FcConfigBuildFonts(config);
				if (!res) {
					LOG_MSG("%s fonts", true, errprefix.c_str());
					InitFailed();
					return false;
				}
			} else {
				// Can't load step by step to use our cache, so retry with general
				// fontconfig init method, that has a few extra fallbacks.

				// Init everything. Normally this would be enough, but the method before
				// accounts for situations where system config is borked due to incompatible
				// lib and system config files.
				FcConfig *fcConfig = FcInitLoadConfigAndFonts();
				if (fcConfig) {
					FcConfigDestroy(config); // release previous config
					config = fcConfig;

					// add our cache at the back of the new config.
					FcConfigParseAndLoadFromMemory(config, reinterpret_cast<const FcChar8*>(configFmtVar.c_str()), FcTrue);
				} else {
					LOG_MSG("%s config and fonts. No system fallbacks will be available", false, errprefix.c_str());
				}
			}

			gameFontSet = FcFontSetCreate();
			basePattern = FcPatternCreate();

			// init app fonts dir
			res = FcConfigAppFontAddDir(config, reinterpret_cast<const FcChar8*>("fonts"));
			if (!res) {
				LOG_MSG("%s font dir", true, errprefix.c_str());
				InitFailed();
				return false;
			}

			// print cache dirs
			auto dirs = FcConfigGetCacheDirs(config);
			FcStrListFirst(dirs);
			for (FcChar8* dir = FcStrListNext(dirs); dir != nullptr; dir = FcStrListNext(dirs)) {
				LOG_MSG("[%s] Using Fontconfig cache dir \"%s\"", false, __func__, dir);
			}
			FcStrListDone(dirs);
		}

		#endif // USE_FONTCONFIG

		return true;
	}

	void InitFailed() {
		FcConfigDestroy(config);
		FcFini();
		config = nullptr;
	}
	static bool InitSingletonFontconfig(bool console) { return singleton->InitFontconfig(console); }

	static bool UseFontConfig() { return (configHandler == nullptr || configHandler->GetBool("UseFontConfigLib")); }

	#ifdef USE_FONTCONFIG
	// command-line CheckGenFontConfigFull invocation checks
	static bool CheckFontConfig() { return (UseFontConfig() && FcConfigUptoDate(GetFCConfig())); }
	#else

	static bool CheckFontConfig() { return false; }
	static bool CheckGenFontConfig(bool fromCons) { return false; }
	#endif

	static FT_Library& GetLibrary() {
		if (singleton == nullptr)
			singleton = std::make_unique<FtLibraryHandler>();

		return singleton->lib;
	};
	static FcConfig* GetFCConfig() {
		if (singleton == nullptr)
			singleton = std::make_unique<FtLibraryHandler>();

		return singleton->config;
	}
	static inline bool CanUseFontConfig() {
		return GetFCConfig() != nullptr;
	}
	#ifdef USE_FONTCONFIG
	static FcFontSet *GetGameFontSet() {
		return singleton->gameFontSet;
	}
	static FcPattern *GetBasePattern() {
		return singleton->basePattern;
	}
	static void ClearGameFontSet() {
		FcFontSetDestroy(singleton->gameFontSet);
		singleton->gameFontSet = FcFontSetCreate();
	}
	static void ClearBasePattern() {
		FcPatternDestroy(singleton->basePattern);
		singleton->basePattern = FcPatternCreate();
	}
	#endif
private:
	FcConfig* config;
	FT_Library lib;
	#ifdef USE_FONTCONFIG
	FcFontSet *gameFontSet;
	FcPattern *basePattern;
	#endif

	static inline std::unique_ptr<FtLibraryHandler> singleton = nullptr;
};
#endif



void FtLibraryHandlerProxy::InitFtLibrary()
{
	RECOIL_DETAILED_TRACY_ZONE;
#ifndef HEADLESS
	FtLibraryHandler::GetLibrary();
#endif
}

bool FtLibraryHandlerProxy::InitFontconfig(bool console)
{
	RECOIL_DETAILED_TRACY_ZONE;
#ifndef HEADLESS
	return FtLibraryHandler::InitSingletonFontconfig(console);
#else
	return false;
#endif
}



/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/

#ifndef HEADLESS
static inline uint64_t GetKerningHash(char32_t lchar, char32_t rchar)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (lchar < 128 && rchar < 128)
		return (lchar << 7) | rchar; // 14bit used

	return (static_cast<uint64_t>(lchar) << 32) | static_cast<uint64_t>(rchar); // 64bit used
}

static std::shared_ptr<FontFace> LoadFontFace(const std::string& fontfile)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(CFontTexture::sync.GetThreadSafety() || Threading::IsMainThread());
	auto lock = CFontTexture::sync.GetScopedLock();

	// get the file (no need to cache, takes too little time)
	std::string fontPath(fontfile);
	CFileHandler f(fontPath);

	if (!f.FileExists()) {
		// check in 'fonts/', too
		if (fontPath.substr(0, 6) != "fonts/") {
			f.Close();
			f.Open(fontPath = "fonts/" + fontPath);
		}

		if (!f.FileExists())
			throw content_error("Couldn't find font '" + fontfile + "'.");
	}

	// we need to keep a copy of the memory
	const int filesize = f.FileSize();

	std::weak_ptr<FontFileBytes>& fontMemWeak = fontMemCache[fontPath];
	std::shared_ptr<FontFileBytes> fontMem = fontMemWeak.lock();

	if (fontMemWeak.expired()) {
		fontMem = std::make_shared<FontFileBytes>(FontFileBytes(filesize));
		f.Read(fontMem.get()->data(), filesize);
		fontMemWeak = fontMem;
	}

	// load the font
	FT_Face face_ = nullptr;
	FT_Error error = FT_New_Memory_Face(FtLibraryHandler::GetLibrary(), fontMem.get()->data(), filesize, 0, &face_);
	auto face = spring::ScopedResource(
		face_,
		[](FT_Face f) { if (f) FT_Done_Face(f); }
	);

	if (error != 0) {
		throw content_error(fmt::format("FT_New_Face failed: {}", GetFTError(error)));
	}

	return std::make_shared<FontFace>(face.Release(), fontMem);
}

static std::shared_ptr<FontFace> GetRenderFontFace(const std::string& fontfile, int size)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(CFontTexture::sync.GetThreadSafety() || Threading::IsMainThread());
	auto lock = CFontTexture::sync.GetScopedLock();

	//TODO add support to load fonts by name (needs fontconfig)

	FT_Error error;

	const auto fontKey = fontfile + IntToString(size);
	const auto fontIt = fontFaceCache.find(fontKey);

	if (fontIt != fontFaceCache.end() && !fontIt->second.expired())
		return fontIt->second.lock();

	std::shared_ptr<FontFace> facePtr = LoadFontFace(fontfile);

	// set render size
	if (!FT_IS_SCALABLE(facePtr->face) && facePtr->face->num_fixed_sizes >= 1)
		size = static_cast<unsigned int>(facePtr->face->available_sizes[0].y_ppem / 64.0);

	if ((error = FT_Set_Pixel_Sizes(facePtr->face, 0, size)) != 0) {
		throw content_error(fmt::format("FT_Set_Pixel_Sizes failed: {}", GetFTError(error)));
	}

	// select unicode charmap
	if ((error = FT_Select_Charmap(facePtr->face, FT_ENCODING_UNICODE)) != 0) {
		throw content_error(fmt::format("FT_Select_Charmap failed: {}", GetFTError(error)));
	}
	return (fontFaceCache[fontKey] = facePtr).lock();
}
#endif



#ifndef HEADLESS

inline
static std::string GetFaceKey(FT_Face f)
{
	RECOIL_DETAILED_TRACY_ZONE;
	FT_FaceRec_* fr = static_cast<FT_FaceRec_*>(f);
	return fmt::format("{}-{}-{}", fr->family_name, fr->style_name, fr->num_glyphs);
}

// NOLINTNEXTLINE{misc-misplaced-const}
template<typename USET>
static std::shared_ptr<FontFace> GetFontForCharacters(const std::vector<char32_t>& characters, const FT_Face origFace, const int origSize, const USET& blackList)
{
	RECOIL_DETAILED_TRACY_ZONE;
#if defined(USE_FONTCONFIG)
	if (characters.empty())
		return nullptr;

	if (!FtLibraryHandler::CanUseFontConfig())
		return nullptr;

	// create list of wanted characters
	auto cset = spring::ScopedResource(
		FcCharSetCreate(),
		[](FcCharSet* cs) { if (cs) FcCharSetDestroy(cs); }
	);

	for (auto c: characters) {
		FcCharSetAddChar(cset, c);
	}

	// create properties of the wanted font starting from our priorities pattern.
	auto pattern = spring::ScopedResource(
		FcPatternDuplicate(FtLibraryHandler::GetBasePattern()),
		[](FcPattern* p) { if (p) FcPatternDestroy(p); }
	);

	{
		{
			FcValue v;
			v.type = FcTypeBool;
			v.u.b = FcTrue;
			FcPatternAddWeak(pattern, FC_ANTIALIAS, v, FcFalse);
		}

		FcPatternAddCharSet(pattern, FC_CHARSET   , cset);
		FcPatternAddBool(   pattern, FC_SCALABLE  , FcTrue);
		FcPatternAddDouble( pattern, FC_SIZE      , static_cast<double>(origSize));

		double pixelSize = 0.0;
		int weight = FC_WEIGHT_NORMAL;
		int slant  = FC_SLANT_ROMAN;
		FcBool outline = FcFalse;

		FcChar8* family = nullptr;

		const FcChar8* ftname = reinterpret_cast<const FcChar8*>("not used");

		auto blanks = spring::ScopedResource(
			FcBlanksCreate(),
			[](FcBlanks* b) { if (b) FcBlanksDestroy(b); }
		);

		auto origPattern = spring::ScopedResource(
			FcFreeTypeQueryFace(origFace, ftname, 0, blanks),
			[](FcPattern* p) { if (p) FcPatternDestroy(p); }
		);

		if (origPattern != nullptr) {
			FcPatternGetInteger(origPattern, FC_WEIGHT    , 0, &weight );
			FcPatternGetInteger(origPattern, FC_SLANT     , 0, &slant  );
			FcPatternGetBool(   origPattern, FC_OUTLINE   , 0, &outline);
			FcPatternGetDouble( origPattern, FC_PIXEL_SIZE, 0, &pixelSize);

			FcPatternGetString( origPattern, FC_FAMILY , 0, &family );

		}

		FcPatternAddInteger(pattern, FC_WEIGHT, weight);
		FcPatternAddInteger(pattern, FC_SLANT, slant);
		FcPatternAddBool(pattern, FC_OUTLINE, outline);

		if (pixelSize > 0.0)
			FcPatternAddDouble(pattern, FC_PIXEL_SIZE, pixelSize);

		if (family)
			FcPatternAddString(pattern, FC_FAMILY, family);
	}

	FcDefaultSubstitute(pattern);
	if (!FcConfigSubstitute(FtLibraryHandler::GetFCConfig(), pattern, FcMatchPattern))
	{
		return nullptr;
	}

	// search fonts that fit our request
	typedef std::unique_ptr<FcFontSet, decltype(&FcFontSetDestroy)> ScopedFcFontSet;

	int nFonts = 0;
	bool loadMore = true;
	FcResult res;

	// first search game fonts
	FcFontSet *sets[] = { FtLibraryHandler::GetGameFontSet() };
	ScopedFcFontSet fs(FcFontSetSort(FtLibraryHandler::GetFCConfig(), sets, 1, pattern, FcFalse, nullptr, &res), &FcFontSetDestroy);

	if (fs != nullptr && res == FcResultMatch)
		nFonts = fs->nfont;

	// iterate returned font list, and perform system font search when in need of more fonts
	int i = 0;
	while (i < nFonts || loadMore) {
		if (i == nFonts) {
			// now search system fonts
			fs = ScopedFcFontSet(FcFontSort(FtLibraryHandler::GetFCConfig(), pattern, FcFalse, nullptr, &res), &FcFontSetDestroy);
			if (fs == nullptr || res != FcResultMatch)
				return nullptr;
			loadMore = false;
			nFonts = fs->nfont;
			i = 0;
		}
		const FcPattern* font = fs->fonts[i++];

		FcChar8* cFilename = nullptr;
		FcResult r = FcPatternGetString(font, FC_FILE, 0, &cFilename);
		if (r != FcResultMatch || cFilename == nullptr)
			continue;

		const std::string filename = std::string{ reinterpret_cast<char*>(cFilename) };

		if (invalidFonts.find(std::make_pair(filename, origSize)) != invalidFonts.end()) //this font is known to error out
			continue;

		try {
			auto face = GetRenderFontFace(filename, origSize);

			if (blackList.find(GetFaceKey(*face)) != blackList.cend())
				continue;

			#ifdef _DEBUG
			{
				std::ostringstream ss;
				for (auto c : characters) {
					ss << "<" << static_cast<uint32_t>(c) << ">";
				}
				LOG_L(L_INFO, "[%s] Using \"%s\" to render chars (size=%d) %s", __func__, filename.c_str(), origSize, ss.str().c_str());
			}
			#endif

			return face;
		}
		catch (content_error& ex) {
			invalidFonts.emplace(filename, origSize);
			LOG_L(L_WARNING, "[%s] \"%s\" (s = %d): %s", __func__, filename.c_str(), origSize, ex.what());
			continue;
		}

	}
	return nullptr;
#else
	return nullptr;
#endif
}
#endif


/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/


CFontTexture::CFontTexture(const std::string& fontfile, int size, int _outlinesize, float  _outlineweight)
	: outlineSize(_outlinesize)
	, outlineWeight(_outlineweight)
	, lineHeight(0)
	, fontDescender(0)
	, fontSize(size)
	, texWidth(0)
	, texHeight(0)
	, wantedTexWidth(0)
	, wantedTexHeight(0)
	, needsColor(false)
	, isColor(false)
{
	RECOIL_DETAILED_TRACY_ZONE;
	atlasAlloc.SetMaxSize(globalRendering->maxTextureSize, globalRendering->maxTextureSize);

	atlasGlyphs.reserve(1024);

	if (fontSize <= 0)
		fontSize = 14;

	fontFamily = "unknown";
	fontStyle  = "unknown";

	fontRenderer = CglFontRenderer::CreateInstance();
#ifndef HEADLESS

	try {
		shFace = GetRenderFontFace(fontfile, fontSize);
	}
	catch (content_error& ex) {
		LOG_L(L_ERROR, "[%s] %s (s=%d): %s", __func__, fontfile.c_str(), fontSize, ex.what());
		return;
	}

	if (shFace == nullptr)
		return;

	FT_Face face = *shFace;

	static constexpr int FT_INTERNAL_DPI = 64;
	normScale = 1.0f / (fontSize * FT_INTERNAL_DPI);
	float pixScale = 1.0;
	bool canScale = false;

	if (!FT_IS_SCALABLE(shFace->face)) {
		if (shFace->face->num_fixed_sizes > 0) {
			pixScale = std::floor(1.0 / (shFace->face->available_sizes[0].y_ppem * normScale));
			canScale = true;
		} else {
			LOG_L(L_WARNING, "[%s] %s is not scalable and not reported sizes", __func__, fontfile.c_str());
			normScale = 1.0f;
		}

	}

	if (!FT_HAS_KERNING(face)) {
		LOG_L(L_INFO, "[%s] %s has no kerning data", __func__, fontfile.c_str());
	}

	fontFamily = face->family_name;
	fontStyle  = face->style_name;

	fontDescender = normScale * FT_MulFix(face->descender, face->size->metrics.y_scale);
	//lineHeight = FT_MulFix(face->height, face->size->metrics.y_scale); // bad results
	lineHeight = face->height / face->units_per_EM;

	if (lineHeight <= 0)
		lineHeight = 1.25 * (face->bbox.yMax - face->bbox.yMin);

	// TODO: untested
	if (canScale) {
		fontDescender = fontDescender*(pixScale/normScale);
		//fontDescender = fontDescender*pixScale ??
		lineHeight = lineHeight*pixScale;
	}

	// has to be done before first GetGlyph() call!
	CreateTexture(32, 32, true);

	// precache ASCII glyphs & kernings (save them in kerningPrecached array for better lvl2 cpu cache hitrate)
	PreloadGlyphs();

#endif
}

/***
 *
 * Preloads standard alphabet glyphs for a font
 */
void CFontTexture::PreloadGlyphs()
{
#ifndef HEADLESS
	FT_Face face = *shFace;
	// don't preload for non alphanumeric
	if (!FT_Get_Char_Index(face, 65))
		return;
	//preload Glyphs
	// if given face doesn't contain alphanumerics, don't preload it
	if (!FT_Get_Char_Index(face, 'a'))
		return;
	LoadWantedGlyphs(32, 127);
	for (char32_t i = 32; i < 127; ++i) {
		const auto& lgl = GetGlyph(i);
		const float advance = lgl.advance;
		for (char32_t j = 32; j < 127; ++j) {
			const auto& rgl = GetGlyph(j);
			const auto hash = GetKerningHash(i, j);
			FT_Vector kerning = {};
			if (FT_HAS_KERNING(face))
				FT_Get_Kerning(face, lgl.index, rgl.index, FT_KERNING_DEFAULT, &kerning);

			// TODO: likely need rescaling for not rescalable fonts
			kerningPrecached[hash] = advance + normScale * kerning.x;
		}
	}
#endif
}

CFontTexture::~CFontTexture()
{
	RECOIL_DETAILED_TRACY_ZONE;
	CglFontRenderer::DeleteInstance(fontRenderer);
#ifndef HEADLESS
	glDeleteTextures(1, &glyphAtlasTextureID);
	glyphAtlasTextureID = 0;
#endif
}

/***
 *
 * Add a fallback font
 *
 * @param fontfile VFS path for the font
 */
bool CFontTexture::AddFallbackFont(const std::string& fontfile)
{
#if defined(USE_FONTCONFIG) && !defined(HEADLESS)
	if (!FtLibraryHandler::CanUseFontConfig())
		return false;

	FcFontSet *set = FtLibraryHandler::GetGameFontSet();

	// Check if font already loaded
	for (int i=0; set && i < set->nfont; ++i) {
		FcPattern* font = set->fonts[i];
		FcChar8 *file;
		if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch) {
			if (fontfile.compare(reinterpret_cast<const char*>(file)) == 0) {
				return true;
			}
		}
	}

	// Load font face
	std::shared_ptr<FontFace> facePtr;
	try {
		facePtr = LoadFontFace(fontfile);
	} catch (content_error& ex) {
		LOG_L(L_ERROR, "[%s] \"%s\": %s", __func__, fontfile.c_str(), ex.what());
		return false;
	}

	// Add fontconfig configuration
	FT_Face face = *facePtr;

	// Store pattern
	FcPattern* pattern = FcFreeTypeQueryFace(face, reinterpret_cast<const FcChar8*>(fontfile.c_str()), 0, NULL);
	if (!FcFontSetAdd(set, pattern))
	{
		LOG_L(L_WARNING, "[%s] could not add pattern for %s", __func__, fontfile.c_str());
		return false;
	}
	// needed?:
	//FcConfigSubstitute(FtLibraryHandler::GetFCConfig(), pattern, FcMatchScan);

	// Add to priority fonts pattern
	FcChar8* family = nullptr;
	if (FcPatternGetString( pattern, FC_FAMILY , 0, &family ) == FcResultMatch) {
		FcPattern *basePattern = FtLibraryHandler::GetBasePattern();
		FcPatternAddString(basePattern, FC_FAMILY, family);
	} else {
		LOG_L(L_WARNING, "[%s] could not add priority for %s", __func__, fontfile.c_str());
		return false;
	}

	needsClearGlyphs = true;

	return true;
#else
	return false;
#endif
}

/***
 *
 * Clears fontconfig fallbacks
 */
void CFontTexture::ClearFallbackFonts()
{
#if defined(USE_FONTCONFIG) && !defined(HEADLESS)
	if (!FtLibraryHandler::CanUseFontConfig())
		return;

	FtLibraryHandler::ClearBasePattern();
	FtLibraryHandler::ClearGameFontSet();

	needsClearGlyphs = true;
#endif
}

/***
 *
 * Clears all glyphs for all fonts
 */
void CFontTexture::ClearAllGlyphs() {
#ifndef HEADLESS
	RECOIL_DETAILED_TRACY_ZONE;

	bool changed = false;
	for (const auto& ft : allFonts) {
		auto lf = ft.lock();
		changed |= lf->ClearGlyphs();
	}
	if (changed)
		eventHandler.FontsChanged();

	needsClearGlyphs = false;
#endif
}

/***
 *
 * Clears all glyphs for a font
 */
bool CFontTexture::ClearGlyphs() {
	RECOIL_DETAILED_TRACY_ZONE;

	bool changed = false;
#ifndef HEADLESS

	// Invalidate glyphs coming from other fonts, or those with the 'not found' glyph.
	for (const auto& g : glyphs) {
		if (g.second.face->face != shFace->face || g.second.index == 0) {
			changed = true;
		}
	}

	// Always clear failed attempts in case we have any cache here.
	failedAttemptsToReplace.clear();

	if (changed) {
		kerningPrecached = {};

		// clear all glyps
		glyphs.clear();

		// clear atlases
		ClearAtlases(32, 32);

		// preload standard glyphs
		PreloadGlyphs();

		// signal need to update texture
		++curTextureUpdate;
	}
#endif
	return changed;
}

void CFontTexture::InitFonts()
{
	RECOIL_DETAILED_TRACY_ZONE;
#ifndef HEADLESS
	maxFontTries = configHandler ? configHandler->GetInt("MaxFontTries") : 5;
#endif
}

void CFontTexture::KillFonts()
{
	RECOIL_DETAILED_TRACY_ZONE;
	// check unused fonts
	std::erase_if(allFonts, [](std::weak_ptr<CFontTexture> item) { return item.expired(); });

	assert(allFonts.empty());
	allFonts = {}; //just in case
}

void CFontTexture::Update() {
	// called from Game::UpdateUnsynced
	assert(CFontTexture::sync.GetThreadSafety() || Threading::IsMainThread());
	auto lock = CFontTexture::sync.GetScopedLock();

	// check unused fonts
	std::erase_if(allFonts, [](std::weak_ptr<CFontTexture> item) { return item.expired(); });

	static std::vector<std::shared_ptr<CFontTexture>> fontsToUpdate;
	fontsToUpdate.clear();

	if (needsClearGlyphs)
		ClearAllGlyphs();

	for (const auto& font : allFonts) {
		auto lf = font.lock();
		if (lf->GlyphAtlasTextureNeedsUpdate() || lf->GlyphAtlasTextureNeedsUpload())
			fontsToUpdate.emplace_back(std::move(lf));
	}

	for_mt_chunk(0, fontsToUpdate.size(), [](int i) {
		fontsToUpdate[i]->UpdateGlyphAtlasTexture();
	});

	for (const auto& font : fontsToUpdate)
		font->UploadGlyphAtlasTexture();

	fontsToUpdate.clear();
}

const GlyphInfo& CFontTexture::GetGlyph(char32_t ch)
{
	RECOIL_DETAILED_TRACY_ZONE;
#ifndef HEADLESS
	if (const auto it = glyphs.find(ch); it != glyphs.end())
		return it->second;
#endif

	return dummyGlyph;
}


float CFontTexture::GetKerning(const GlyphInfo& lgl, const GlyphInfo& rgl)
{
	RECOIL_DETAILED_TRACY_ZONE;
#ifndef HEADLESS
	if (!FT_HAS_KERNING(shFace->face))
		return lgl.advance;

	// first check caches
	const uint64_t hash = GetKerningHash(lgl.letter, rgl.letter);

	if (hash < kerningPrecached.size())
		return kerningPrecached[hash];

	const auto it = kerningDynamic.find(hash);

	if (it != kerningDynamic.end())
		return it->second;

	if (lgl.face != rgl.face)
		return (kerningDynamic[hash] = lgl.advance);

	// load & cache
	FT_Vector kerning;
	FT_Get_Kerning(*lgl.face, lgl.index, rgl.index, FT_KERNING_DEFAULT, &kerning);
	// TODO: likely need rescaling for not rescalable fonts
	return (kerningDynamic[hash] = lgl.advance + normScale * kerning.x);
#else
	return 0;
#endif
}

void CFontTexture::LoadWantedGlyphs(char32_t begin, char32_t end)
{
	RECOIL_DETAILED_TRACY_ZONE;
	static std::vector<char32_t> wanted;
	wanted.clear();
	for (char32_t i = begin; i < end; ++i)
		wanted.emplace_back(i);

	LoadWantedGlyphs(wanted);
}

void CFontTexture::LoadWantedGlyphs(const std::vector<char32_t>& wanted)
{
	RECOIL_DETAILED_TRACY_ZONE;
#ifndef HEADLESS
	if (wanted.empty())
		return;

	assert(CFontTexture::sync.GetThreadSafety() || Threading::IsMainThread());
	auto lock = CFontTexture::sync.GetScopedLock();

	static std::vector<char32_t> map;
	map.clear();

	for (auto c : wanted) {
		if (auto it = failedAttemptsToReplace.find(c); (it != failedAttemptsToReplace.end() && it->second == maxFontTries))
			continue;

		auto it = std::lower_bound(nonPrintableRanges.begin(), nonPrintableRanges.end(), c);
		if (it != nonPrintableRanges.end() && !(c < *it)) {
			LoadGlyph(shFace, c, 0);
			failedAttemptsToReplace.emplace(c, maxFontTries);
		}
		else {
			map.emplace_back(c);
			// instantiate on the first retry to save space
			//failedAttemptsToReplace.emplace(c, 0);
		}
	}
	spring::VectorSortUnique(map);

	if (map.empty())
		return;

	// load glyphs from different fonts (using fontconfig)
	std::shared_ptr<FontFace> f = shFace;

	static spring::unordered_set<std::string> alreadyCheckedFonts;
	alreadyCheckedFonts.clear();
	do {
		alreadyCheckedFonts.insert(GetFaceKey(*f));

		for (std::size_t idx = 0; idx < map.size(); /*nop*/) {
			if (auto it = failedAttemptsToReplace.find(map[idx]); (it != failedAttemptsToReplace.end() && it->second == maxFontTries)) {
				// handle maxFontTries attempts case
				LoadGlyph(shFace, map[idx], 0);
				LOG_L(L_WARNING, "[CFontTexture::%s] Failed to load glyph %u after %d font replacement attempts", __func__, uint32_t(map[idx]), failedAttemptsToReplace[map[idx]]);
				map[idx] = map.back();
				map.pop_back();
				continue;
			}

			FT_UInt index = FT_Get_Char_Index(*f, map[idx]);

			if (index != 0) {
				LoadGlyph(f, map[idx], index);

				map[idx] = map.back();
				map.pop_back();
			}
			else {
				++failedAttemptsToReplace[map[idx]];
				++idx;
			}
		}
		f = GetFontForCharacters(map, *f, fontSize, alreadyCheckedFonts);
	} while (!map.empty() && f);

	// handle glyphs that didn't reach maxFontTries number of attempts, but nonetheless failed
	for (auto c: map) {
		LoadGlyph(shFace, c, 0);
		LOG_L(L_WARNING, "[CFontTexture::%s] Failed to load glyph %u after %d font replacement attempts", __func__, uint32_t(c), failedAttemptsToReplace[c]);
	}

	// read atlasAlloc glyph data back into atlasUpdate{Shadow}
	{
		if (!atlasAlloc.Allocate())
			LOG_L(L_WARNING, "[CFontTexture::%s] Texture limit reached! (try to reduce the font size and/or outlinewidth)", __func__);

		wantedTexWidth  = atlasAlloc.GetAtlasSize().x;
		wantedTexHeight = atlasAlloc.GetAtlasSize().y;

		if ((atlasUpdate.xsize != wantedTexWidth) || (atlasUpdate.ysize != wantedTexHeight))
			atlasUpdate = atlasUpdate.CanvasResize(wantedTexWidth, wantedTexHeight, false);

		if (needsColor && !isColor) {
			CBitmap newAtlas = {};
			newAtlas.Alloc(wantedTexWidth, wantedTexHeight, needsColor ? 4 : 1);
			newAtlas.CopySubImage(atlasUpdate, 0, 0);
			atlasUpdate = newAtlas;
		}

		if (atlasUpdateShadow.Empty()) {
			atlasUpdateShadow.Alloc(wantedTexWidth, wantedTexHeight, needsColor ? 4 : 1);
		} else if (needsColor && !isColor) {
			CBitmap newAtlas = {};
			newAtlas.Alloc(atlasUpdateShadow.xsize, atlasUpdateShadow.ysize, needsColor ? 4 : 1);
			newAtlas.CopySubImage(atlasUpdateShadow, 0, 0);
			atlasUpdateShadow = newAtlas;
		}

		if ((atlasUpdateShadow.xsize != wantedTexWidth) || (atlasUpdateShadow.ysize != wantedTexHeight))
			atlasUpdateShadow = atlasUpdateShadow.CanvasResize(wantedTexWidth, wantedTexHeight, false);

		for (const auto i : wanted) {
			const std::string glyphName  = IntToString(i);
			const std::string glyphName2 = glyphName + "sh";

			if (!atlasAlloc.contains(glyphName))
				continue;

			const auto texpos  = atlasAlloc.GetEntry(glyphName);
			const auto texpos2 = atlasAlloc.GetEntry(glyphName2);

			//glyphs is a map
			auto& thisGlyph = glyphs[i];

			thisGlyph.texCord       = IGlyphRect(texpos [0], texpos [1], texpos [2] - texpos [0], texpos [3] - texpos [1]);
			thisGlyph.shadowTexCord = IGlyphRect(texpos2[0], texpos2[1], texpos2[2] - texpos2[0], texpos2[3] - texpos2[1]);

			const size_t glyphIdx = reinterpret_cast<size_t>(atlasAlloc.GetEntryData(glyphName));

			assert(glyphIdx < atlasGlyphs.size());

			if (texpos[2] != 0)
				atlasUpdate.CopySubImage(atlasGlyphs[glyphIdx], texpos.x, texpos.y);
			if (texpos2[2] != 0)
				atlasUpdateShadow.CopySubImage(atlasGlyphs[glyphIdx], texpos2.x + outlineSize, texpos2.y + outlineSize);
		}

		atlasAlloc.clear();
		atlasGlyphs.clear();
	}

	// schedule a texture update
	++curTextureUpdate;
#endif
}



void CFontTexture::LoadGlyph(std::shared_ptr<FontFace>& f, char32_t ch, unsigned index)
{
	RECOIL_DETAILED_TRACY_ZONE;
#ifndef HEADLESS
	if (glyphs.find(ch) != glyphs.end())
		return;

	// check for duplicated glyphs
	const auto pred = [&](const std::pair<char32_t, GlyphInfo>& p) { return (p.second.index == index && *p.second.face == f->face); };
	const auto iter = std::find_if(glyphs.begin(), glyphs.end(), pred);

	if (iter != glyphs.end()) {
		auto glyphInfo = iter->second;
		glyphs[ch] = glyphInfo;
		return;
	}

	auto& glyph = glyphs[ch];
	glyph.face  = f;
	glyph.index = index;
	glyph.letter = ch;

	// load glyph
	auto flags = FT_LOAD_DEFAULT;
	if (FT_HAS_COLOR(f->face)) {
		flags |= FT_LOAD_COLOR;
	} else {
		flags |= FT_LOAD_RENDER;
	}
	if (FT_Load_Glyph(*f, index, flags) != 0)
		LOG_L(L_ERROR, "Couldn't load glyph %d", ch);

	FT_GlyphSlot slot = f->face->glyph;

	if (slot->bitmap.pixel_mode == FT_PIXEL_MODE_BGRA)
		needsColor = true;

	const float xbearing = slot->metrics.horiBearingX * normScale;
	const float ybearing = slot->metrics.horiBearingY * normScale;

	glyph.size.x = xbearing;
	glyph.size.y = ybearing - fontDescender;
	glyph.size.w =  slot->metrics.width * normScale;
	glyph.size.h = -slot->metrics.height * normScale;

	glyph.advance   = slot->advance.x * normScale;
	glyph.height    = slot->metrics.height * normScale;
	glyph.descender = ybearing - glyph.height;

	// workaround bugs in FreeSansBold (in range 0x02B0 - 0x0300)
	if (glyph.advance == 0 && glyph.size.w > 0)
		glyph.advance = glyph.size.w;

	int width  = slot->bitmap.width;
	int height = slot->bitmap.rows;
	const int olSize = 2 * outlineSize;
	const int channels = slot->bitmap.pixel_mode == FT_PIXEL_MODE_BGRA ? 4 : 1;

	if (slot->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY && slot->bitmap.pixel_mode != FT_PIXEL_MODE_BGRA) {
		LOG_L(L_ERROR, "invalid pixeldata mode %d %d", slot->bitmap.pixel_mode, FT_HAS_COLOR(f->face));
		return;
	}

	if (slot->bitmap.pitch != width*channels) {
		LOG_L(L_ERROR, "invalid pitch %d (width %d channels %d)", slot->bitmap.pitch, width, channels);
		return;
	}

	// store glyph bitmap (index) in allocator until the next LoadWantedGlyphs call
	if (!FT_IS_SCALABLE(f->face)) {
		CBitmap temp = CBitmap(slot->bitmap.buffer, width, height, channels);
		const float pixSize = static_cast<float>(f->face->available_sizes[0].y_ppem);
		const float ratio = 1.0/(pixSize*normScale);

		width = std::floor(width*ratio);
		height = std::floor(height*ratio);
		glyph.advance = glyph.advance*ratio;
		glyph.descender = glyph.descender*ratio;
		glyph.size.y = glyph.size.y*ratio;
		glyph.size.y = ybearing*ratio - fontDescender;
		glyph.size.x = glyph.size.x*ratio;
		glyph.size.w = glyph.size.w*ratio;
		glyph.size.h = glyph.size.h*ratio;
		glyph.height = glyph.height*ratio;

		atlasGlyphs.emplace_back(temp.CreateRescaled(width, height));
	}
	else
		atlasGlyphs.emplace_back(slot->bitmap.buffer, width, height, channels);

	atlasAlloc.AddEntry(IntToString(ch)       , int2(width         , height         ), reinterpret_cast<void*>(atlasGlyphs.size() - 1));
	atlasAlloc.AddEntry(IntToString(ch) + "sh", int2(width + olSize, height + olSize)                                                 );
#endif
}

void CFontTexture::CreateTexture(const int width, const int height, const bool init)
{
	RECOIL_DETAILED_TRACY_ZONE;
#ifndef HEADLESS
	if (init)
		glGenTextures(1, &glyphAtlasTextureID);
	glBindTexture(GL_TEXTURE_2D, glyphAtlasTextureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	// no border to prevent artefacts in outlined text
	constexpr GLfloat borderColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};

	// NB:
	// The modern and core formats like GL_R8 and GL_RED are intentionally replaced with
	// deprecated GL_ALPHA, such that AMD-HACK users could enjoy no-shader fallback
	// But why fallback? See: https://github.com/beyond-all-reason/spring/issues/383
	// Remove the code under `#ifdef SUPPORT_AMD_HACKS_HERE` blocks throughout this file
	// when all potatoes die.

#ifdef SUPPORT_AMD_HACKS_HERE
	constexpr GLint swizzleMaskF[] = { GL_ALPHA, GL_ALPHA, GL_ALPHA, GL_ALPHA };
	constexpr GLint swizzleMaskD[] = { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA };
	if (needsColor)
		glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMaskD);
	else
		glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMaskF);
#endif
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	if (needsColor)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
	else
#ifdef SUPPORT_AMD_HACKS_HERE
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 1, 1, 0, GL_ALPHA, GL_UNSIGNED_BYTE, nullptr);
#else
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, 1, 1, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
#endif

	glBindTexture(GL_TEXTURE_2D, 0);
#ifdef SUPPORT_AMD_HACKS_HERE
	if (!needsColor)
		glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMaskD);
#endif

	if (init) {
		atlasUpdate = {};
		atlasUpdate.Alloc(texWidth = wantedTexWidth = width, texHeight = wantedTexHeight = height, needsColor ? 4 : 1);

		atlasUpdateShadow = {};
		atlasUpdateShadow.Alloc(width, height, needsColor ? 4 : 1);
	}
#endif
}

void CFontTexture::ReallocAtlases(bool pre)
{
	RECOIL_DETAILED_TRACY_ZONE;
#ifndef HEADLESS
	static std::vector<uint8_t> atlasMem;
	static std::vector<uint8_t> atlasShadowMem;
	static int2 atlasDim;
	static int2 atlasUDim;

	if (pre) {
		assert(!atlasUpdate.Empty());

		atlasMem.clear();
		atlasMem.resize(atlasUpdate.GetMemSize());

		atlasShadowMem.clear();
		atlasShadowMem.resize(atlasUpdateShadow.GetMemSize());

		memcpy(atlasMem.data(), atlasUpdate.GetRawMem(), atlasUpdate.GetMemSize());
		memcpy(atlasShadowMem.data(), atlasUpdateShadow.GetRawMem(), atlasUpdateShadow.GetMemSize());

		atlasDim = { atlasUpdate.xsize, atlasUpdate.ysize };
		atlasUDim = { atlasUpdateShadow.xsize, atlasUpdateShadow.ysize };

		atlasUpdate = {};
		atlasUpdateShadow = {};
		return;
	}

	// NB: pool has already been wiped here, do not return memory to it but just realloc
	atlasUpdate.Alloc(atlasDim.x, atlasDim.y, needsColor ? 4 : 1);
	atlasUpdateShadow.Alloc(atlasUDim.x, atlasUDim.y, needsColor ? 4 : 1);

	memcpy(atlasUpdate.GetRawMem(), atlasMem.data(), atlasMem.size());
	memcpy(atlasUpdateShadow.GetRawMem(), atlasShadowMem.data(), atlasShadowMem.size());


	if (atlasGlyphs.empty()) {
		atlasMem = {};
		atlasShadowMem = {};
		atlasDim = {};
		atlasUDim = {};
		return;
	}

	LOG_L(L_WARNING, "[FontTexture::%s] discarding %u glyph bitmaps", __func__, uint32_t(atlasGlyphs.size()));

	// should be empty, but realloc glyphs just in case so we can safely dispose of them
	for (CBitmap& bmp: atlasGlyphs) {
		bmp.Alloc(1, 1, 1);
	}

	atlasGlyphs.clear();

	atlasMem = {};
	atlasShadowMem = {};
	atlasDim = {};
	atlasUDim = {};
#endif
}

void CFontTexture::ClearAtlases(const int width, const int height)
{
#ifndef HEADLESS
	// refresh the atlasAlloc to reset coordinates
	atlasAlloc = CRowAtlasAlloc();
	atlasAlloc.SetMaxSize(globalRendering->maxTextureSize, globalRendering->maxTextureSize);

	// clear atlases
	wantedTexWidth = width;
	wantedTexHeight = height;

	atlasUpdate.Alloc(wantedTexWidth, wantedTexHeight, needsColor ? 4 : 1);
	atlasUpdateShadow.Alloc(1, 1, needsColor ? 4 : 1);
	atlasUpdateShadow = {};

	if (!atlasGlyphs.empty())
		LOG_L(L_WARNING, "[FontTexture::%s] discarding %u glyph bitmaps", __func__, uint32_t(atlasGlyphs.size()));

	atlasGlyphs.clear();
#endif
}

bool CFontTexture::GlyphAtlasTextureNeedsUpdate() const
{
	RECOIL_DETAILED_TRACY_ZONE;
#ifndef HEADLESS
	return curTextureUpdate != lastTextureUpdate;
#else
	return false;
#endif
}

bool CFontTexture::GlyphAtlasTextureNeedsUpload() const
{
	RECOIL_DETAILED_TRACY_ZONE;
#ifndef HEADLESS
	return needsTextureUpload;
#else
	return false;
#endif
}

void CFontTexture::UpdateGlyphAtlasTexture()
{
	RECOIL_DETAILED_TRACY_ZONE;
#ifndef HEADLESS
	// no need to lock, MT safe
	if (!GlyphAtlasTextureNeedsUpdate())
		return;

	if (needsColor && !isColor)
		needsTextureUpload = true;

	lastTextureUpdate = curTextureUpdate;
	texWidth  = wantedTexWidth;
	texHeight = wantedTexHeight;

	// merge shadow and regular atlas bitmaps, dispose shadow
	if (atlasUpdateShadow.xsize == atlasUpdate.xsize && atlasUpdateShadow.ysize == atlasUpdate.ysize) {
		atlasUpdateShadow.Blur(outlineSize, outlineWeight);
		assert((atlasUpdate.xsize * atlasUpdate.ysize) % sizeof(int) == 0);

		const int channels = needsColor ? 4 : 1;

		const int* src = reinterpret_cast<const int*>(atlasUpdateShadow.GetRawMem());
		      int* dst = reinterpret_cast<      int*>(atlasUpdate.GetRawMem());

		const int size = (atlasUpdate.xsize * atlasUpdate.ysize * channels) / sizeof(int);

		assert(atlasUpdateShadow.GetMemSize() / sizeof(int) == size);
		assert(atlasUpdate.GetMemSize() / sizeof(int) == size);

		for (int i = 0; i < size; ++i) {
			dst[i] |= src[i];
		}

		atlasUpdateShadow = {}; // MT-safe
		needsTextureUpload = true;
	}

#endif
}

void CFontTexture::UploadGlyphAtlasTexture()
{
	RECOIL_DETAILED_TRACY_ZONE;
	fontRenderer->HandleTextureUpdate(*this, true);
}

void CFontTexture::UploadGlyphAtlasTextureImpl()
{
	RECOIL_DETAILED_TRACY_ZONE;
#ifndef HEADLESS
	if (!GlyphAtlasTextureNeedsUpload())
		return;
	if (needsColor && !isColor) {
		CreateTexture(32, 32, false);
		isColor = true;
		needsTextureUpload = true;
	}

	// update texture atlas
	glBindTexture(GL_TEXTURE_2D, glyphAtlasTextureID);
	if (needsColor)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texWidth, texHeight, 0, GL_BGRA,  GL_UNSIGNED_BYTE, atlasUpdate.GetRawMem());
	else
	#ifdef SUPPORT_AMD_HACKS_HERE
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, texWidth, texHeight, 0, GL_ALPHA, GL_UNSIGNED_BYTE, atlasUpdate.GetRawMem());
	#else
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, texWidth, texHeight, 0, GL_RED, GL_UNSIGNED_BYTE, atlasUpdate.GetRawMem());
	#endif
	glBindTexture(GL_TEXTURE_2D, 0);

	needsTextureUpload = false;
#endif
}

FT_Byte* FontFileBytes::data()
{
	RECOIL_DETAILED_TRACY_ZONE;
	return vec.data();
}

FontFace::FontFace(FT_Face f, std::shared_ptr<FontFileBytes>& mem)
	: face(f)
	, memory(mem)
{ }

FontFace::~FontFace()
{
	RECOIL_DETAILED_TRACY_ZONE;
#ifndef HEADLESS
	FT_Done_Face(face);
#endif
}

FontFace::operator FT_Face()
{
	RECOIL_DETAILED_TRACY_ZONE;
	return this->face;
}
