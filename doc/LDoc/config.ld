project='Spring Lua API'
title='Spring Lua API documentation'
description='A Lua documentation tool'
not_luadoc=true
no_space_before_args=true
format='markdown'
file={
  '../../rts/Lua/LuaSyncedCtrl.cpp',
  '../../rts/Lua/LuaUnsyncedCtrl.cpp',
  '../../rts/Lua/LuaSyncedRead.cpp',
  '../../rts/Lua/LuaUnsyncedRead.cpp',
  '../../rts/Lua/LuaSyncedMoveCtrl.cpp',
  '../../rts/Lua/LuaHandle.cpp',
  '../../rts/Lua/LuaHandleSynced.cpp',
  '../../rts/Lua/LuaVFS.cpp',
  '../../rts/Lua/LuaVAO.cpp',
  '../../rts/Lua/LuaVAOImpl.cpp',
  '../../rts/Lua/LuaVBO.cpp',
  '../../rts/Lua/LuaVBOImpl.cpp',
  '../../rts/Lua/LuaOpenGL.cpp',
  '../../rts/Lua/LuaZip.cpp',
  '../../rts/Lua/LuaRules.cpp',
  '../../rts/Lua/LuaMetalMap.cpp',
  '../../rts/Lua/LuaShaders.cpp',
  '../../rts/Lua/LuaFBOs.cpp',
  '../../rts/Lua/LuaRBOs.cpp',
  '../../rts/Lua/LuaMathExtra.cpp',
  '../../rts/Lua/LuaBitOps.cpp',
  '../../rts/Lua/LuaConstGame.cpp',
  '../../rts/Lua/LuaConstEngine.cpp',
  '../../rts/Lua/LuaConstPlatform.cpp',
  '../../rts/Lua/LuaConstCMD.cpp',
  '../../rts/Lua/LuaConstCMDTYPE.cpp',
  '../../rts/Lua/LuaConstCOB.cpp',
  '../../rts/Lua/LuaConstGL.cpp',
}
dir='../site/ldoc'
template='./'

local repoPath = "https://github.com/beyond-all-reason/spring/blob/BAR105/%s"

custom_see_handler('^(rts/.*)$', function(filename)
    local url = repoPath:format(filename)
    return filename, url
end)
