/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef IMODELPARSER_H
#define IMODELPARSER_H

#include <vector>
#include <string>
#include <mutex>
#include <condition_variable>

#include "3DModel.h"
#include "System/UnorderedMap.hpp"


class IModelParser
{
public:
	virtual ~IModelParser() = default;
	virtual void Init() {}
	virtual void Kill() {}
	virtual void Load(S3DModel& model, const std::string& name) = 0;
};


class CModelLoader
{
public:
	void Init();
	void Kill();

	S3DModel* LoadModel(std::string name, bool preload = false);
	std::string FindModelPath(std::string name) const;

	bool IsValid() const { return (!parsers.empty()); }
	void PreloadModel(const std::string& name);
	void LogErrors();

	void DrainPreloadFutures(uint32_t numAllowed = 0);

	const std::vector<S3DModel>& GetModelsVec() const { return models; }
	      std::vector<S3DModel>& GetModelsVec()       { return models; }
private:
	void ParseModel(S3DModel& model, const std::string& name, const std::string& path);
	void FillModel(S3DModel& model, const std::string& name, const std::string& path);
	S3DModel* GetCachedModel(std::string name);

	IModelParser* GetFormatParser(const std::string& pathExt);

	void InitParsers() const;
	void KillModels();
	void KillParsers() const;

	void PostProcessGeometry(S3DModel* o);
	void Upload(S3DModel* o) const;

private:
	std::vector<std::pair<std::string, uint32_t>> cache; // "<fullpath>/armflash.3do" --> idx at models
	std::vector<std::pair<std::string, IModelParser*>> parsers;

	std::condition_variable_any cv;

	//can't be weak_ptr here, because in that case there are no owners left for futures. preloadFutures needs to own futures
	std::vector<std::shared_future<void>> preloadFutures;

	std::vector<S3DModel> models;
	std::vector< std::pair<std::string, std::string> > errors;

	// all unique models loaded so far
	uint32_t modelID = 0;
public:
	using ParsersType = decltype(parsers);
};

extern CModelLoader modelLoader;

#endif /* IMODELPARSER_H */
