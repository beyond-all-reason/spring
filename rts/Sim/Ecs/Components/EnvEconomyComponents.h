#ifndef ENV_ECONOMY_COMPONENTS_H__
#define ENV_ECONOMY_COMPONENTS_H__

namespace EnvEconomy {

// holds values that always matter to a wind generator
struct WindGenerator {
};

// indicates that this is a wind generator that needs to be given a
// starting wind value while the next wind update is coming.
struct NewWindGenerator {
};

// Only present when the wind generator is active
struct WindGeneratorActive {
};

}

#endif