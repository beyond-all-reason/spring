#include "ParticleGeneratorHandler.h"


void ParticleGeneratorHandler::Init()
{
	static auto Initialize = [](auto& gen) mutable {
		using T = std::remove_reference<decltype(*gen.get())>::type;
		gen = std::make_unique<T>();
	};
	
	std::apply([](auto& ... gens) mutable {
		(Initialize(gens), ...);
	}, generators);
}

void ParticleGeneratorHandler::Kill()
{
	static auto Nullify = [](auto& gen) mutable {
		gen = nullptr;
	};
	
	std::apply([](auto& ... gens) mutable {
		(Nullify(gens), ...);
	}, generators);
}

void ParticleGeneratorHandler::GenerateAll()
{
	static auto Generate = [](auto& gen) mutable {
		gen->Generate();
	};
	
	std::apply([](auto& ... gens) mutable {
		(Generate(gens), ...);
	}, generators);
}
