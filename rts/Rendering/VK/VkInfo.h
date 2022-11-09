#pragma once

class VkInfo {
public:
	static void PrintInfo();
private:
	static void PrintInfoImpl(const char* funcName);
};