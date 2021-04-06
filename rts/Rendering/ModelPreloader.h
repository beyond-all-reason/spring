#ifndef MODEL_PRELOADER_H
#define MODEL_PRELOADER_H

class ModelPreloader {
public:
	static void Load() {
		// map features are loaded earlier in featureHandler.LoadFeaturesFromMap(); - not a big deal

		LoadUnitDefs();
		LoadFeatureDefs();
		LoadWeaponDefs();

		// after that point we should've loaded all models, it's time to dispatch VBO/EBO/VAO creation
	}
private:
	static void LoadUnitDefs();
	static void LoadFeatureDefs();
	static void LoadWeaponDefs();
};

#endif // MODEL_PRELOADER_H