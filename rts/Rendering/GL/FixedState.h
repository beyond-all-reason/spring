#ifndef FIXED_STATE_H
#define FIXED_STATE_H

#include <functional>
#include <string>
#include <unordered_map>
#include <stack>
#include <tuple>

#include "Rendering/GL/myGL.h"

class FixedPipelineState {
private:
    template <typename Result, typename ...Args> using vararg_function = std::function< Result(Args...) >;
    using glEnum1Func = vararg_function<void, GLenum>;
    using glEnum2Func = vararg_function<void, GLenum, GLenum>;
    using glFloat1Func = vararg_function<void, GLfloat>;
    using glFloat2Func = vararg_function<void, GLfloat, GLfloat>;
public:
    FixedPipelineState();

    FixedPipelineState& PolygonMode(const GLenum mode) { return CommonNamedState(__func__, glPolygonMode, GL_FRONT_AND_BACK, mode); }

    FixedPipelineState& DepthMask(const bool b) { return CommonNamedState(__func__, glDepthMask, b); }
    FixedPipelineState& DepthRange(const GLfloat n, const GLfloat f) { return CommonNamedState(__func__, glDepthRangef, n, f); }
    FixedPipelineState& DepthTest(const bool b) { return CommonBinaryState(GL_DEPTH_TEST, b); }
    FixedPipelineState& DepthFunc(const GLenum func) { return CommonNamedState(__func__, glDepthFunc, func); }

    FixedPipelineState& Blending(const bool b) { return CommonBinaryState(GL_BLEND, b); }
    FixedPipelineState& BlendFunc(const GLenum sfactor, const GLenum dfactor) { return CommonNamedState(__func__, glBlendFunc, sfactor, dfactor); }

    FixedPipelineState& Multisampling(const bool b) { return CommonBinaryState(GL_MULTISAMPLE, b); }
    /// TODO
public:
    void Bind() const;
    void Unbind() const;
private:
    FixedPipelineState& CommonBinaryState(const GLenum state, bool b) {
        binaryStates[state] = b;
        return *this;
    }

    FixedPipelineState& CommonNamedState(const char* name, glEnum1Func&& f, bool b) {
        named1EnumStates[std::string(name)] = std::make_tuple(f, (b ? GL_TRUE : GL_FALSE));
        return *this;
    }

    FixedPipelineState& CommonNamedState(const char* name, glEnum1Func&& f, GLenum v) {
        named1EnumStates[std::string(name)] = std::make_tuple(f, v);
        return *this;
    }

    FixedPipelineState& CommonNamedState(const char* name, glEnum2Func&& f, GLenum v1, GLenum v2) {
        named2EnumStates[std::string(name)] = std::make_tuple(f, v1, v2);
        return *this;
    }

    FixedPipelineState& CommonNamedState(const char* name, glFloat1Func&& f, GLfloat v) {
        named1FloatStates[std::string(name)] = std::make_tuple(f, v);
        return *this;
    }

    FixedPipelineState& CommonNamedState(const char* name, glFloat2Func&& f, GLfloat v1, GLfloat v2) {
        named2FloatStates[std::string(name)] = std::make_tuple(f, v1, v2);
        return *this;
    }

private:
    std::unordered_map<GLenum, bool> binaryStates;

    std::unordered_map<std::string, std::tuple<glEnum1Func, GLenum>> named1EnumStates;
    std::unordered_map<std::string, std::tuple<glEnum2Func, GLenum, GLenum>> named2EnumStates;

    std::unordered_map<std::string, std::tuple<glEnum1Func, GLfloat>> named1FloatStates;
    std::unordered_map<std::string, std::tuple<glEnum2Func, GLfloat, GLfloat>> named2FloatStates;
private:
    static std::stack<FixedPipelineState> statesChain;
};
std::stack<FixedPipelineState> FixedPipelineState::statesChain = {};

/*
class ScopedState {
public:
    ScopedState(const FixedPipelineState& state) : s{ state } { s.Bind(); }
    ~ScopedState() { s.Unbind(); }
private:
    const FixedPipelineState& s;
};
*/

#endif