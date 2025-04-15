#pragma once

#include <string>
#include <string_view>
#include <stddef.h>
#include <memory>

#include "System/Transform.hpp"
#include "System/TypeToStr.h"
#include "Rendering/GL/StreamBuffer.h"
#include "Rendering/Models/ModelsMemStorageDefs.h"


class CUnit;
class CFeature;
class CProjectile;
struct UnitDef;
struct FeatureDef;
struct S3DModel;

class TransformsUploader {
public:
	using MyClassName = TransformsUploader;
	using MyDataType = Transform;
public:
	void Init();
	void Kill();
	void Update();
public:
	uint32_t GetElemsCount() const { return ssbo->GetByteSize() / sizeof(MyDataType); }
	constexpr uint32_t GetBindingIdx() const { return MATRIX_SSBO_BINDING_IDX; }
	constexpr uint32_t GetElemCountIncr() const { return ELEM_COUNTI; }
public:
	// Defs
	size_t GetElemOffset(const UnitDef* def) const;
	size_t GetElemOffset(const FeatureDef* def) const;
	size_t GetElemOffset(const S3DModel* model) const;
	size_t GetUnitDefElemOffset(int32_t unitDefID) const;
	size_t GetFeatureDefElemOffset(int32_t featureDefID) const;

	// Objs
	size_t GetElemOffset(const CUnit* unit) const;
	size_t GetElemOffset(const CFeature* feature) const;
	size_t GetElemOffset(const CProjectile* proj) const;
	size_t GetUnitElemOffset(int32_t unitID) const;
	size_t GetFeatureElemOffset(int32_t featureID) const;
	size_t GetProjectileElemOffset(int32_t syncedProjectileID) const;
private:
	std::unique_ptr<IStreamBuffer<MyDataType>> ssbo;
private:
	static constexpr const char* className = spring::TypeToCStr<MyClassName>();
	static constexpr uint32_t MATRIX_SSBO_BINDING_IDX = 0;
	static constexpr uint32_t ELEM_COUNT0 = 1u << 16;
	static constexpr uint32_t ELEM_COUNTI = 1u << 14;
};

class ModelUniformsUploader {
public:
	using MyClassName = ModelUniformsUploader;
	using MyDataType = ModelUniformData;
public:
	void Init();
	void Kill();
	void Update();
public:
	uint32_t GetElemsCount() const { return ssbo->GetByteSize() / sizeof(MyDataType); }
	constexpr uint32_t GetBindingIdx() const { return MATUNI_SSBO_BINDING_IDX; }
	constexpr uint32_t GetElemCountIncr() const { return ELEM_COUNTI; }
public:
	// Defs
	size_t GetElemOffset(const UnitDef* def) const;
	size_t GetElemOffset(const FeatureDef* def) const;
	size_t GetElemOffset(const S3DModel* model) const;
	size_t GetUnitDefElemOffset(int32_t unitDefID) const;
	size_t GetFeatureDefElemOffset(int32_t featureDefID) const;

	// Objs
	size_t GetElemOffset(const CUnit* unit) const;
	size_t GetElemOffset(const CFeature* feature) const;
	size_t GetElemOffset(const CProjectile* proj) const;
	size_t GetUnitElemOffset(int32_t unitID) const;
	size_t GetFeatureElemOffset(int32_t featureID) const;
	size_t GetProjectileElemOffset(int32_t syncedProjectileID) const;
private:
	std::unique_ptr<IStreamBuffer<MyDataType>> ssbo;
private:
	static constexpr const char* className = spring::TypeToCStr<MyClassName>();
	static constexpr uint32_t MATUNI_SSBO_BINDING_IDX = 1;
	static constexpr uint32_t ELEM_COUNT0 = 1u << 12;
	static constexpr uint32_t ELEM_COUNTI = 1u << 11;
};

extern TransformsUploader transformsUploader;
extern ModelUniformsUploader modelUniformsUploader;