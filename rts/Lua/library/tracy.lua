---@meta

--Types for tracy
--https://github.com/wolfpld/tracy/blame/master/public/tracy/TracyLua.hpp

tracy = {}

---@param name string
function tracy.ZoneBegin(name) end

---@param name string
function tracy.ZoneBeginN(name) end

---@param name string
function tracy.ZoneBeginS(name) end

---@param name string
function tracy.ZoneBeginNS(name) end

function tracy.ZoneEnd() end

---@param text string
function tracy.ZoneText(text) end

---@param name string
function tracy.ZoneName(name) end

---@param text string
function tracy.Message(text) end