#include "CobFileHandler.h"
#include "System/FileSystem/FileHandler.h"

#include <tracy/Tracy.hpp>

CCobFile* CCobFileHandler::GetCobFile(const std::string& name)
{
	//ZoneScoped;
	const auto it = cobFileHandles.find(name);

	if (it != cobFileHandles.end())
		return &cobFileObjects[it->second];

	CFileHandler f(name);

	if (!f.FileExists())
		return nullptr;

	cobFileHandles[name] = cobFileObjects.size();
	cobFileObjects.emplace_back(CCobFile(f, name));

	return &cobFileObjects[cobFileObjects.size() - 1];
}


CCobFile* CCobFileHandler::ReloadCobFile(const std::string& name)
{
	//ZoneScoped;
	const auto it = cobFileHandles.find(name);

	if (it == cobFileHandles.end())
		return (GetCobFile(name));

	CFileHandler f(name);
	assert(f.FileExists());

	cobFileObjects[it->second] = CCobFile(f, name);
	return &cobFileObjects[it->second];
}


const CCobFile* CCobFileHandler::GetScriptFile(const std::string& name) const
{
	//ZoneScoped;
	const auto it = cobFileHandles.find(name);

	if (it != cobFileHandles.end())
		return &cobFileObjects[it->second];

	return nullptr;
}

