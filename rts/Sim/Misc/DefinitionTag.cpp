/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "DefinitionTag.h"
#include "System/Log/ILog.h"
#include "System/StringUtil.h"
#include <iostream>

#include <tracy/Tracy.hpp>

using std::cout;


/**
 * @brief Log an error about a DefTagMetaData
 */
#define LOG_VAR(data, fmt, ...) \
	LOG_L(L_ERROR, "%s:%d: " fmt, (data)->GetDeclarationFile().Get().c_str(), (data)->GetDeclarationLine().Get(), ## __VA_ARGS__) \


DefType::DefType(const char* n): name(n) {
	metaDataMem.fill(0);
	defInitFuncs.fill(nullptr);
	tagMetaData.fill(nullptr);
	GetTypes().push_back(this);
}


void DefType::AddTagMetaData(const DefTagMetaData* data)
{
	//ZoneScoped;
	const auto key = data->GetInternalName();

	const auto tend = tagMetaData.begin() + tagMetaDataCnt;
	const auto pred = [&](const DefTagMetaData* md) { return (key == md->GetInternalName()); };
	const auto iter = std::find_if(tagMetaData.begin(), tend, pred);

	if (iter != tend) {
		LOG_VAR(data, "Duplicate config variable declaration \"%s\"", key.c_str());
		LOG_VAR(*iter, "  Previously declared here");
		assert(false);
		return;
	}
	if (tagMetaDataCnt >= tagMetaData.size()) {
		LOG_VAR(data, "Too many config-variable metadata instances");
		return;
	}

	tagMetaData[tagMetaDataCnt++] = data;

	if (const std::string internalKey = StringToLower(data->GetInternalName());
			!tagMetaDataByInternalName.contains(internalKey)) {
		tagMetaDataByInternalName[internalKey] = data;
	}

	if (const auto& externalName = data->GetExternalName(); externalName.IsSet()) {
		if (const std::string externalKey = StringToLower(externalName.Get());
				!tagMetaDataByExternalName.contains(externalKey)) {
			tagMetaDataByExternalName[externalKey] = data;
		}
	}
	if (const auto& fallbackName = data->GetFallbackName(); fallbackName.IsSet()) {
		if (const std::string fallbackKey = StringToLower(fallbackName.Get());
				!tagMetaDataByFallbackName.contains(fallbackKey)) {
			tagMetaDataByFallbackName[fallbackKey] = data;
		}
	}
}


const DefTagMetaData* DefType::GetMetaDataByInternalKey(const string& key) {
	//ZoneScoped;
	const std::string lkey = StringToLower(key);
	if (auto it = tagMetaDataByInternalName.find(lkey);
			it != tagMetaDataByInternalName.end()) {
		return it->second;
	}
	return nullptr;
}


const DefTagMetaData* DefType::GetMetaDataByExternalKey(const string& key) {
	//ZoneScoped;
	const std::string lkey = StringToLower(key);
	if (auto it = tagMetaDataByExternalName.find(lkey);
			it != tagMetaDataByExternalName.end()) {
		return it->second;
	}
	if (auto it = tagMetaDataByInternalName.find(lkey);
			it != tagMetaDataByInternalName.end()) {
		return it->second;
	}
	if (auto it = tagMetaDataByFallbackName.find(lkey);
			it != tagMetaDataByFallbackName.end()) {
		return it->second;
	}

	return nullptr;
}

/**
 * @brief Call Quote if type is not bool, float or int.
 */
static inline std::string Quote(const std::string& type, const std::string& value)
{
	if (type == spring::TypeToStr<std::string>())
		return Quote(value);

	return value;
}


/**
 * @brief Write a DefTagMetaData to a stream.
 */
static std::ostream& operator<< (std::ostream& out, const DefTagMetaData* d)
{
	//ZoneScoped;
	const char* const OUTER_INDENT = "    ";
	const char* const INDENT = "      ";

	const std::string tname = d->GetTypeName();

	out << OUTER_INDENT << Quote(d->GetKey()) << ": {\n";

#define KV(key, value) out << INDENT << Quote(#key) << ": " << (value) << ",\n"

	if (d->GetDeclarationFile().IsSet())
		KV(declarationFile, Quote(d->GetDeclarationFile().Get()));

	if (d->GetDeclarationLine().IsSet())
		KV(declarationLine, d->GetDeclarationLine().Get());

	if (d->GetExternalName().IsSet())
		KV(internalName, Quote(d->GetInternalName()));

	if (d->GetFallbackName().IsSet())
		KV(fallbackName, Quote(d->GetFallbackName().Get()));

	if (d->GetDescription().IsSet())
		KV(description, Quote(d->GetDescription().Get()));

	if (d->GetDefaultValue().IsSet())
		KV(defaultValue, Quote(tname, d->GetDefaultValue().ToString()));

	if (d->GetMinimumValue().IsSet())
		KV(minimumValue, Quote(tname, d->GetMinimumValue().ToString()));

	if (d->GetMaximumValue().IsSet())
		KV(maximumValue, Quote(tname, d->GetMaximumValue().ToString()));

	if (d->GetScaleValue().IsSet())
		KV(scaleValue, Quote(tname, d->GetScaleValue().ToString()));

	if (d->GetScaleValueStr().IsSet())
		KV(scaleValueString, Quote(d->GetScaleValueStr().ToString()));

	if (d->GetTagFunctionStr().IsSet())
		KV(tagFunction, Quote(d->GetTagFunctionStr().ToString()));

	// Type is required.
	// Easiest to do this last because of the trailing comma that isn't there.
	out << INDENT << Quote("type") << ": " << Quote(tname) << "\n";

#undef KV

	out << OUTER_INDENT << "}";

	return out;
}

/**
 * @brief Output config variable meta data as JSON to stdout.
 *
 * This can be tested using, for example:
 *
 *	./spring --list-def-tags |
 *		python -c 'import json, sys; json.dump(json.load(sys.stdin), sys.stdout)'
 */
void DefType::OutputMetaDataMap() const
{
	//ZoneScoped;
	cout << "{\n";

	bool first = true;

	for (unsigned int i = 0; i < tagMetaDataCnt; i++) {
		const DefTagMetaData* md = tagMetaData[i];

		if (!first)
			cout << ",\n";

		cout << md;
		first = false;
	}

	cout << "\n  }";
}

void DefType::OutputTagMap()
{
	//ZoneScoped;
	cout << "{\n";

	bool first = true;
	for (const DefType* defType: GetTypes()) {
		if (!first)
			cout << ",\n";

		cout << "  " << Quote(defType->GetName()) << ": ";
		defType->OutputMetaDataMap();
		first = false;
	}

	cout << "\n}\n";
}


void DefType::CheckType(const DefTagMetaData* meta, const std::string_view otherTypeName)
{
	//ZoneScoped;
	assert(meta != nullptr);
	if (meta->GetTypeName() != otherTypeName)
		LOG_L(L_ERROR, "DEFTAG \"%s\" defined with wrong typevalue \"%s\" should be \"%s\"", meta->GetKey().c_str(), meta->GetTypeName().c_str(), otherTypeName.data());
}


void DefType::ReportUnknownTags(const std::string& instanceName, const LuaTable& luaTable, const std::string pre)
{
	//ZoneScoped;
	std::vector<std::string> keys;
	luaTable.GetKeys(keys);

	for (const std::string& tag: keys) {
		const DefTagMetaData* meta = GetMetaDataByExternalKey(pre + tag);

		if (meta != nullptr)
			continue;

		if (luaTable.GetType(tag) == LuaTable::TABLE) {
			ReportUnknownTags(instanceName, luaTable.SubTable(tag), pre + tag + ".");
			continue;
		}

		LOG_L(L_WARNING, "%s: Unknown tag \"%s%s\" in \"%s\"", name, pre.c_str(), tag.c_str(), instanceName.c_str());
	}
}


void DefType::Load(void* instance, const LuaTable& luaTable)
{
	//ZoneScoped;
	this->luaTable = &luaTable;

	for (unsigned int i = 0; i < defInitFuncCnt; i++) {
		defInitFuncs[i](instance);
	}

	this->luaTable = nullptr;
}
