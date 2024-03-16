#include "ModelsMemStorageDefs.h"

#include <map>
#include <string>
#include <sstream>
#include <optional>

#include "fmt/format.h"

#include <tracy/Tracy.hpp>

CR_BIND(ModelUniformData, )
CR_REG_METADATA(ModelUniformData, (
	CR_MEMBER_BEGINFLAG(CM_NoSerialize),

		CR_MEMBER(composite),
		CR_MEMBER(unused2),
		CR_MEMBER(unused3),
		CR_MEMBER(unused4),
		CR_MEMBER(maxHealth),
		CR_MEMBER(health),
		CR_MEMBER(unused5),
		CR_MEMBER(unused6),
		CR_MEMBER(drawPos),
		CR_MEMBER(speed),
		CR_MEMBER(userDefined),
	CR_MEMBER_ENDFLAG(CM_NoSerialize)
))


void ModelUniformData::SetGLSLDefinition(int binding)
{
	//ZoneScoped;
	const ModelUniformData dummy{};

	std::map<uint32_t, std::pair<std::string, std::string>> membersMap;
	for (const auto& member : dummy.GetClass()->members) {
		membersMap[member.offset] = std::make_pair(std::string{ member.name }, member.type->GetName());
	}

	std::ostringstream output;

	output << fmt::format("layout(std140, binding = {}) readonly buffer {} {{\n", binding, dummy.GetClass()->name); // {{ - escaped {

	for (const auto& [offset, info] : membersMap) {
		const auto& [name, tname] = info;

		std::string glslType;
		if (tname.rfind("CMatrix44f") != std::string::npos)
			glslType = "mat4";
		else if (tname.rfind("float4") != std::string::npos)
			glslType = "vec4";
		else if (tname.rfind("float3") != std::string::npos)
			glslType = "vec3";
		else if (tname.rfind("float2") != std::string::npos)
			glslType = "vec2";
		else if (tname.rfind("int") != std::string::npos)
			glslType = "uint";
		else if (tname.rfind("float") != std::string::npos)
			glslType = "float";


		std::optional<int> arraySize;
		{
			const size_t bro = tname.rfind("[");
			const size_t brc = tname.rfind("]");

			if (bro != std::string::npos && brc != std::string::npos) {
				std::string arraySizeStr = tname.substr(bro + 1, brc - bro);
				arraySize = std::stoi(arraySizeStr);
			}
		}

		if (arraySize.has_value()) {
			if (glslType == "uint") {
				glslType = "uvec4";
			}
			else if (glslType == "float") {
				glslType = "vec4";
			}
			else {
				assert(false);
			}
			arraySize = arraySize.value() / 4;
		}

		assert(!glslType.empty());
		if (arraySize.value_or(1) == 1)
			output << fmt::format("\t{} {};\n", glslType, name);
		else
			output << fmt::format("\t{} {}[{}];\n", glslType, name, arraySize.value());
	}

	output << "};\n";

	glslDefinition = output.str();
}