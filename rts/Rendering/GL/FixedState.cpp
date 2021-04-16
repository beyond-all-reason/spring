#include "FixedState.h"

FixedPipelineState::FixedPipelineState()
{
	if (statesChain.empty()) { //default state
		statesChain.push(*this); //reserve 1st slot to avoid recursive calls
		statesChain.top()
			.DepthTest(false)
			.DepthTest(false); //TODO
	} else {
		*this = statesChain.top(); //copy&paste previos state
	}
}

void FixedPipelineState::Bind() const
{
	/*
	std::unordered_map<std::string, std::tuple<glEnum1Func, GLenum>> named1EnumStates;
	std::unordered_map<std::string, std::tuple<glEnum2Func, GLenum, GLenum>> named2EnumStates;

	std::unordered_map<std::string, std::tuple<glEnum1Func, GLfloat>> named1FloatStates;
	std::unordered_map<std::string, std::tuple<glEnum2Func, GLfloat, GLfloat>> named2FloatStates;

	std::unordered_map<GLenum, bool> binaryStates;
	*/

	for (const auto& named1EnumState : named1EnumStates) {

	}

}

void FixedPipelineState::Unbind() const
{

}
