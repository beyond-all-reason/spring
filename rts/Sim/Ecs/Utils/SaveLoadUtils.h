#ifndef SAVE_LOAD_UTILS_H__
#define SAVE_LOAD_UTILS_H__

#include "Sim/Ecs/EcsMain.h"
#include "System/creg/Serializer.h"

namespace SystemGlobals {

class SaveLoadUtils {
public:
    static void LoadComponents(creg::CInputStreamSerializer &os, std::stringstream &oss);
    static void SaveComponents(creg::COutputStreamSerializer &os, std::stringstream &oss);
};

}

#endif