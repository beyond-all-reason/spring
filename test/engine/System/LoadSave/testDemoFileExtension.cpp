/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include <zlib.h>

#include <catch_amalgamated.hpp>

#include "System/Config/ConfigHandler.h"
#include "System/LoadSave/DemoFileExtension.h"
#include "System/LoadSave/demofile.h"

// Minimal ConfigHandler stub — just enough for GetDemoFileExtensions()
class StubConfigHandler : public ConfigHandler {
public:
	std::map<std::string, std::string> values;

	void FinalizeLoad() override {}
	void SetString(const std::string&, const std::string&, bool, bool) override {}
	std::string GetString(const std::string& key) const override
	{
		auto it = values.find(key);
		return (it != values.end()) ? it->second : std::string{};
	}
	bool IsSet(const std::string&) const override { return true; }
	bool IsReadOnly(const std::string&) const override { return false; }
	bool IsDeprecated(const std::string&) const override { return false; }
	void Delete(const std::string&) override {}
	std::string GetConfigFile() const override { return {}; }
	const std::map<std::string, std::string> GetData() const override { return {}; }
	std::map<std::string, std::string> GetDataWithoutDefaults() const override { return {}; }
	void Update() override {}
	void EnableWriting(bool) override {}
protected:
	void AddObserver(ConfigNotifyCallback, void*, const std::vector<std::string>&) override {}
	void RemoveObserver(void*) override {}
};

ConfigHandler* configHandler = nullptr;

struct TempGzFile {
	std::string path;

	explicit TempGzFile(const void* data, int len)
	{
		path = std::tmpnam(nullptr);
		gzFile f = gzopen(path.c_str(), "wb");
		REQUIRE(f != nullptr);
		if (len > 0)
			gzwrite(f, data, len);
		gzclose(f);
	}

	~TempGzFile() { std::remove(path.c_str()); }

	TempGzFile(const TempGzFile&) = delete;
	TempGzFile& operator=(const TempGzFile&) = delete;
};

struct DemoExtFixture {
	StubConfigHandler stub;

	DemoExtFixture() { configHandler = &stub; }
	~DemoExtFixture() { configHandler = nullptr; }
};

// ============================= GetDemoFileExtensions ========================

TEST_CASE_METHOD(DemoExtFixture, "GetDemoFileExtensions: single default extension", "[DemoFileExtension]")
{
	stub.values["DemoFileExtension"] = "sdfz";
	auto exts = GetDemoFileExtensions();
	REQUIRE(exts.size() == 1);
	CHECK(exts[0] == "sdfz");
}

TEST_CASE_METHOD(DemoExtFixture, "GetDemoFileExtensions: multiple extensions", "[DemoFileExtension]")
{
	stub.values["DemoFileExtension"] = "barreplay,sdfz";
	auto exts = GetDemoFileExtensions();
	REQUIRE(exts.size() == 2);
	CHECK(exts[0] == "barreplay");
	CHECK(exts[1] == "sdfz");
}

TEST_CASE_METHOD(DemoExtFixture, "GetDemoFileExtensions: trims whitespace", "[DemoFileExtension]")
{
	stub.values["DemoFileExtension"] = "  barreplay , sdfz  ";
	auto exts = GetDemoFileExtensions();
	REQUIRE(exts.size() == 2);
	CHECK(exts[0] == "barreplay");
	CHECK(exts[1] == "sdfz");
}

TEST_CASE_METHOD(DemoExtFixture, "GetDemoFileExtensions: empty string falls back to sdfz", "[DemoFileExtension]")
{
	stub.values["DemoFileExtension"] = "";
	auto exts = GetDemoFileExtensions();
	REQUIRE(exts.size() == 1);
	CHECK(exts[0] == "sdfz");
}

TEST_CASE_METHOD(DemoExtFixture, "GetDemoFileExtensions: skips empty segments", "[DemoFileExtension]")
{
	stub.values["DemoFileExtension"] = "foo,,bar";
	auto exts = GetDemoFileExtensions();
	REQUIRE(exts.size() == 2);
	CHECK(exts[0] == "foo");
	CHECK(exts[1] == "bar");
}

TEST_CASE_METHOD(DemoExtFixture, "GetDemoFileExtensions: filters extensions with slash", "[DemoFileExtension]")
{
	stub.values["DemoFileExtension"] = "bad/ext,ok";
	auto exts = GetDemoFileExtensions();
	REQUIRE(exts.size() == 1);
	CHECK(exts[0] == "ok");
}

TEST_CASE_METHOD(DemoExtFixture, "GetDemoFileExtensions: filters extensions with dot", "[DemoFileExtension]")
{
	stub.values["DemoFileExtension"] = "bad.ext,ok";
	auto exts = GetDemoFileExtensions();
	REQUIRE(exts.size() == 1);
	CHECK(exts[0] == "ok");
}

TEST_CASE_METHOD(DemoExtFixture, "GetDemoFileExtensions: filters extensions with backslash", "[DemoFileExtension]")
{
	stub.values["DemoFileExtension"] = "bad\\ext,ok";
	auto exts = GetDemoFileExtensions();
	REQUIRE(exts.size() == 1);
	CHECK(exts[0] == "ok");
}

TEST_CASE_METHOD(DemoExtFixture, "GetDemoFileExtensions: all invalid falls back to sdfz", "[DemoFileExtension]")
{
	stub.values["DemoFileExtension"] = "a/b,c.d,e\\f";
	auto exts = GetDemoFileExtensions();
	REQUIRE(exts.size() == 1);
	CHECK(exts[0] == "sdfz");
}

// ============================= IsDemoExtension =============================

TEST_CASE_METHOD(DemoExtFixture, "IsDemoExtension: known extensions accepted", "[DemoFileExtension]")
{
	stub.values["DemoFileExtension"] = "barreplay,sdfz";
	CHECK(IsDemoExtension("sdfz"));
	CHECK(IsDemoExtension("barreplay"));
}

TEST_CASE_METHOD(DemoExtFixture, "IsDemoExtension: unknown extension rejected", "[DemoFileExtension]")
{
	stub.values["DemoFileExtension"] = "barreplay,sdfz";
	CHECK_FALSE(IsDemoExtension("mp4"));
	CHECK_FALSE(IsDemoExtension(""));
}

TEST_CASE_METHOD(DemoExtFixture, "IsDemoExtension: case insensitive", "[DemoFileExtension]")
{
	stub.values["DemoFileExtension"] = "sdfz";
	CHECK(IsDemoExtension("SDFZ"));
	CHECK(IsDemoExtension("Sdfz"));
	CHECK(IsDemoExtension("sdfz"));
}

// ========================= ContentsLookLikeAReplay =========================

TEST_CASE("ContentsLookLikeAReplay: valid magic detected", "[DemoFileExtension]")
{
	char magic[16] = {};
	std::memcpy(magic, DEMOFILE_MAGIC, sizeof(magic));
	TempGzFile tmp(magic, sizeof(magic));
	CHECK(ContentsLookLikeAReplay(tmp.path));
}

TEST_CASE("ContentsLookLikeAReplay: wrong magic rejected", "[DemoFileExtension]")
{
	char magic[16] = "not a demofile!";
	TempGzFile tmp(magic, sizeof(magic));
	CHECK_FALSE(ContentsLookLikeAReplay(tmp.path));
}

TEST_CASE("ContentsLookLikeAReplay: too short content rejected", "[DemoFileExtension]")
{
	char data[5] = "spri";
	TempGzFile tmp(data, sizeof(data));
	CHECK_FALSE(ContentsLookLikeAReplay(tmp.path));
}

TEST_CASE("ContentsLookLikeAReplay: empty file rejected", "[DemoFileExtension]")
{
	TempGzFile tmp(nullptr, 0);
	CHECK_FALSE(ContentsLookLikeAReplay(tmp.path));
}

TEST_CASE("ContentsLookLikeAReplay: nonexistent file rejected", "[DemoFileExtension]")
{
	CHECK_FALSE(ContentsLookLikeAReplay("/nonexistent/path/replay.sdfz"));
}
