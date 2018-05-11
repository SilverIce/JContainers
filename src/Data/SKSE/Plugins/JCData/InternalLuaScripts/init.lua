
-- JC supplies these paths and variables
-- path to JCData dir
-- path to JContainers.dll
-- JC's context of type tes_context
local JCDataPath, JCDllPath, JContext = ...

--if (not JCDataPath) and (not JCDllPath) then
--  JCDataPath, JCDllPath = 
--end
--print(JCDataPath, JCDllPath)

---------- Setup Lua, basic Lua, globals




JConstants = {
  DataPath = JCDataPath,
  HeaderPath = JCDataPath .. [[InternalLuaScripts/api_for_lua.h]],
  DllPath = JCDllPath,
  Context = JContext,
}

JC_compileAndRun = nil

---------- MISC. STUFF

local function printTable( t )
  for k,v in pairs(t) do
    print(k,v)
  end
end

local function mixIntoTable(dest, source)
  for k,v in pairs(source) do
    dest[k] = v
  end
end

-- naive implementation which doesn't takes into account
local function copyLuaTable(source)
  local dest = {}
  mixIntoTable(dest, source)
  setmetatable(dest, getmetatable(source))
  return dest
end

-----------------------------------

--printTable(JConstants)

------------------------------------






-- SANDBOXING

--[[
Problems? ^^

1.  Prevent dumb declaring of global variables in JValue.eval*.
1.5 Prevent global variables in user's scripts
2.  Prevent from loading native dlls

--]]

-- JValue.evalLua* sandbox

-- Creates and returns two Lua environments
-- First environment is for Lua scripts, second - for JValue.evalLua scripts
local function createTwoSandboxes()
  -- all JValue.evalLua* scripts sharing the one, immutable sandbox
  local sandbox = {
    -- some standard lua modules and functions
    math = math,
    io = io,
    string = string,
    table = table,
    bit = bit,
        
    --require = nil,
    pairs = pairs,
    ipairs = ipairs,
    next = next,
    error = error,
    assert = assert,
    tonumber = tonumber,
    tostring = tostring,
    type = type,
    next = next,
    print = print,

    pcall = pcall,
    xpcall = xpcall,

    os = {
      time = os.time,
    },

    debug = {
      traceback = debug.traceback,
    },
  }

  local jc = require 'jc'
  -- copy public things from @jc.public into sandbox
  mixIntoTable(sandbox, jc.public)

  -- caches results of module execution
  local user_modules = {}

  -- an alternative to standard lua 'require' function
  -- usage the same: require 'modname.LuaFile'
  local function jc_require (s)
    local mod = user_modules[s]
    if not mod then
      print('trying to load', s)

      local function isFileExists(name)
        local f = io.open(name,"r")
        if f ~= nil then io.close(f) return true else return false end
      end

      -- either jc - jc/init either jc.trololo -  jc/trololo
      local function loadModule(pseudo_path)
        -- replace . with /
        local str = string.gsub(pseudo_path, '%.', [[/]])
        --print('str', str)
        local file_path = JCDataPath .. [[lua/]] .. str .. '.lua'

        if not isFileExists(file_path) then return false end

        local f, message = loadfile (file_path)
        if not f then error(message) end
        setfenv(f, sandbox)
        mod = f()
        user_modules[pseudo_path] = mod
        return true
      end

      local nothing = loadModule(s .. ".init") or loadModule(s) 
    end
    return mod
  end

  sandbox.jrequire = jc_require

  setmetatable(sandbox, {
    __newindex = function(_, _, _) error("attempt to modify script's sandbox") end,
  })

  -- sandbox_2 is environment for JValue.evalLua scripts
  -- Any unknown global variable in this sandbox is treated as link to a module - and __index tries find that module
  local sandbox_2 = copyLuaTable(sandbox)
  setmetatable(sandbox_2, {
    __index = function(self, key) return jc_require(key .. '.init') end,
    __newindex = function(_, _, _) error("attempt to modify script's sandbox") end,
  })

  return sandbox, sandbox_2
end


-------------------------------------------------------------

-- creates evalLua* entry point
-- the function accepts (luaString, handle), compiles (caches) and evaluates luaString
local function makeEvalLuaFunction()

  local jc = require 'jc'
  local wrapJCHandleAsNumber = jc.wrapJCHandleAsNumber
  local returnJCValue = jc.returnJCValue

  local sandbox, evallua_sandbox = createTwoSandboxes()

  -- Caches compiled JValue.evalLua* string (weak cache). Used by .compileAndCache
  local jc_function_cache = {}
  setmetatable(jc_function_cache, {__mode = 'v' })

  -- Caches compiled JValue.evalLua* string (weak cache).
  local function compileAndCache (luaString)
    local func = jc_function_cache[luaString]
    if not func then
      local f, message = loadstring('local args = ...; local jobject = args;' .. luaString)
      if f then
        func = f
        setfenv(f, evallua_sandbox)
        jc_function_cache[luaString] = f
      else
        error(message)
      end
    end
    return func
  end

  return function(luaString, handle)
    local func = compileAndCache(luaString)
    assert(func)
    return returnJCValue( func(wrapJCHandleAsNumber(handle)) )
  end
end


------------------------------------

local function mainRoutine()
  -- do not allow global variable override
  setmetatable(_G, {
    __newindex = function(t, k, v)
      assert(not t[k], "it's not ok to override global variables")
      rawset(t, k, v)
    end
  })

  math.randomseed(os.time())
  -- Setup globals
  package.path = ';' .. JCDataPath .. [[InternalLuaScripts/?.lua;]]


  JC_compileAndRun = makeEvalLuaFunction()
end

mainRoutine()


--JC_compileAndRun('testing.perform()')
