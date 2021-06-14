#ifndef MODEL_PRELOADER_H
#define MODEL_PRELOADER_H

#include "Rendering/Models/3DModel.h"

class ModelPreloader {
public:
	static void Load() {
		// map features are loaded earlier in featureHandler.LoadFeaturesFromMap(); - not a big deal

		LoadUnitDefs();
		LoadFeatureDefs();
		LoadWeaponDefs();

		// after that point we should've loaded all models, it's time to dispatch VBO/EBO/VAO creation
		S3DModelVAO::GetInstance().Init();
	}
private:
	static void LoadUnitDefs();
	static void LoadFeatureDefs();
	static void LoadWeaponDefs();
};

#endif // MODEL_PRELOADER_H