#pragma once

#include <vector>
#include <tuple>
#include <unordered_map>

#include "Rendering/GL/myGL.h"
#include "Rendering/Shaders/Shader.h"

class ModelMaterial {
public:
    using TextureBindings = std::unordered_map<uint8_t, std::tuple<GLenum, GLuint>>; //relSlot, [texType, texId]
public:
    ModelMaterial(TextureBindings textureBindings_, Shader::IProgramObject* po_)
        : textureBindings{textureBindings_}
        , po{po_}
    {};

    /*
    ModelMaterial& operator=(const ModelMaterial& other) { // copy assignment
        if (this == &other) // Guard self assignment
            return *this;

        textureBindings.clear(); textureBindings = other.textureBindings;
        shader = other.shader;
        return *this;
    }

    ModelMaterial& operator=(ModelMaterial&& other) noexcept { //move assignment
        if (this == &other) // Guard self assignment
            return *this;

        textureBindings = std::move(other.textureBindings);
        shader = std::move(other.shader);
        return *this;
    }
    */

    bool operator!=(const ModelMaterial& other) { return !(*this == other); }
    bool operator==(const ModelMaterial& other) {
        if (po != other.po)
            return false;

        return textureBindings == other.textureBindings;
    }
private:
    Shader::IProgramObject* po = nullptr;
    TextureBindings textureBindings;
};