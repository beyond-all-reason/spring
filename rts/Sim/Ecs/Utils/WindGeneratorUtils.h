#ifndef WIND_GENERATOR_UTILS_H__
#define WIND_GENERATOR_UTILS_H__

class CUnit;

class WindGeneratorUtils {
public:
    static void CreateWindGenerator(CUnit *unit);
	static void RemoveWindGenerator(CUnit *unit);

	static void ActivateGenerator(CUnit* unit);
	static void DeactivateGenerator(CUnit* unit);
};

#endif