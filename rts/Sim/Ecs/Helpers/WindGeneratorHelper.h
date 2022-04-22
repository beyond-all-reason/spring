#ifndef WIND_GENERATOR_HELPER_H__
#define WIND_GENERATOR_HELPER_H__

class CUnit;

class WindGeneratorHelper {
public:
    static void CreateWindGenerator(CUnit *unit);
	static void RemoveWindGenerator(CUnit *unit);

	static void ActivateGenerator(CUnit* unit);
	static void DeactivateGenerator(CUnit* unit);
};

#endif