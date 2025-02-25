#include "LuaVAO.h"

#include "lib/sol2/sol.hpp"

#include "LuaVAOImpl.h"
#include "LuaUtils.h"


/******************************************************************************
 *
 * @see rts/Lua/LuaVAO.cpp
******************************************************************************/


bool LuaVAOs::PushEntries(lua_State* L)
{
#if defined(__GNUG__) && defined(_DEBUG)
	const int top = lua_gettop(L);
#endif
	REGISTER_LUA_CFUNC(GetVAO);

	sol::state_view lua(L);
	auto gl = sol::stack::get<sol::table>(L, -1);

	gl.new_usertype<LuaVAOImpl>("VAO",
		sol::constructors<LuaVAOImpl()>(),
		"Delete", &LuaVAOImpl::Delete,

		"AttachVertexBuffer", &LuaVAOImpl::AttachVertexBuffer,
		"AttachInstanceBuffer", &LuaVAOImpl::AttachInstanceBuffer,
		"AttachIndexBuffer", &LuaVAOImpl::AttachIndexBuffer,

		"DrawArrays", &LuaVAOImpl::DrawArrays,
		"DrawElements", &LuaVAOImpl::DrawElements,

		"ClearSubmission", &LuaVAOImpl::ClearSubmission,
		"AddUnitsToSubmission", sol::overload(
			sol::resolve<int(int)>(&LuaVAOImpl::AddUnitsToSubmission),
			sol::resolve<int(const sol::stack_table&)>(&LuaVAOImpl::AddUnitsToSubmission)
		),
		"AddFeaturesToSubmission", sol::overload(
			sol::resolve<int(int)>(&LuaVAOImpl::AddFeaturesToSubmission),
			sol::resolve<int(const sol::stack_table&)>(&LuaVAOImpl::AddFeaturesToSubmission)
		),
		"AddUnitDefsToSubmission", sol::overload(
			sol::resolve<int(int)>(&LuaVAOImpl::AddUnitDefsToSubmission),
			sol::resolve<int(const sol::stack_table&)>(&LuaVAOImpl::AddUnitDefsToSubmission)
		),
		"AddFeatureDefsToSubmission", sol::overload(
			sol::resolve<int(int)>(&LuaVAOImpl::AddFeatureDefsToSubmission),
			sol::resolve<int(const sol::stack_table&)>(&LuaVAOImpl::AddFeatureDefsToSubmission)
		),
		"RemoveFromSubmission", & LuaVAOImpl::RemoveFromSubmission,
		"Submit", &LuaVAOImpl::Submit
	);

	gl.set("VAO", sol::lua_nil); // don't want this to be accessible directly without gl.GetVAO
#if defined(__GNUG__) && defined(_DEBUG)
	lua_settop(L, top); //workaround for https://github.com/ThePhD/sol2/issues/1441, remove when fixed
#endif
	return true;
}

LuaVAOs::~LuaVAOs()
{
	for (auto& lva : luaVAOs) {
		if (lva.expired())
			continue; //destroyed already

		lva.lock()->Delete();
	}
	luaVAOs.clear();
}

/***
 * Example:
 * ```
 * local myVAO = gl.GetVAO()
 * if myVAO == nil then Spring.Echo("Failed to get VAO") end
 * ```
 *
 * @function gl.GetVAO
 * @return VAO? vao The VAO ref on success, else `nil`
 */
int LuaVAOs::GetVAO(lua_State* L)
{
	if (!LuaVAOImpl::Supported()) {
		#ifndef HEADLESS
		LOG_L(L_ERROR, "[LuaVAOs::%s] Important OpenGL extensions are not supported by the system\n  \tGL_ARB_vertex_buffer_object = %d; GL_ARB_vertex_array_object = %d; GL_ARB_instanced_arrays = %d; GL_ARB_draw_elements_base_vertex = %d; GL_ARB_multi_draw_indirect = %d",
			__func__, (GLAD_GL_ARB_vertex_buffer_object), (GLAD_GL_ARB_vertex_array_object), (GLAD_GL_ARB_instanced_arrays), (GLAD_GL_ARB_draw_elements_base_vertex), (GLAD_GL_ARB_multi_draw_indirect)
		);
		#endif

		return 0;
	}

	return sol::stack::call_lua(L, 1, [L]() {
		auto& activeVAOs = CLuaHandle::GetActiveVAOs(L);
		return activeVAOs.luaVAOs.emplace_back(std::make_shared<LuaVAOImpl>()).lock();
	});
}
