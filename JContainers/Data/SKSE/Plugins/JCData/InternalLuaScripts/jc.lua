


--local assert_old = assert
--local function assert(expr, message)
--  if expr == false then print('trace: '..debug.traceback()) end
--  assert_old(expr, message)
--end

----------------------------------------------

local ffi = require("ffi")

local function stripPath(path)
  local limit = 40
  return #path < limit and path or ( '..' .. string.sub(path, -(limit - 1)) )
end

local jclib
do
  -- load declarations
  local function readFileContents(file)
    local f = io.open(file, "rb")
    assert(f, 'no file at: '..file)
    local content = f:read("*all")
    f:close()
    return content
  end
  
  print ('loading C declarations at '..stripPath(JConstants.HeaderPath))

  local declarations = readFileContents(JConstants.HeaderPath)
  assert(declarations and #declarations > 0, 'file ' .. JConstants.HeaderPath .. ' should not be empty')
  ffi.cdef( declarations )
  
  print ('loading dll at '..stripPath(JConstants.DllPath))

  jclib = ffi.load(JConstants.DllPath)
  
  assert('lua, are you there?' == ffi.string(jclib.JC_hello()))
end
-- Load stage End


-- 2. Helper methods to retrieve C++ functions (a functions which were initially made for Papyrus use only)

local function getNativeFunction(className, funcName, returnType, argTypes)
  local funcPtr = jclib.JC_get_c_function(funcName, className)
  local signature = returnType .. '(__cdecl *)(' .. (argTypes or '') .. ')'
  --print('retrieving', funcName)
  return ffi.cast(signature, funcPtr)
end

-- returns {functionName: function} pairs (table)
local function retrieveNativeFunctions(className, functionsTemplate)
  local nativeFuncs = {}
  for func, v in pairs(functionsTemplate) do
    nativeFuncs[func] = getNativeFunction(className, func, v[1], v[2])
  end
  return nativeFuncs
end

-- Helpers methods End

local function readonlytable_error(_,_,_) error('attempt to modify readonly table') end

local function readonlytable(t)
  local mt = getmetatable(t) or {}
  assert(not mt.__newindex, 'already readonly')
  mt.__newindex = readonlytable_error
  setmetatable(t, mt)
  return t
end 

-- Native functions -----------------

local JValueNativeFuncs = retrieveNativeFunctions('JValue',
    {
      count = {'int32_t', 'handle'},
      writeToFile = {'void', 'handle, cstring'},
      readFromFile = {'handle', 'cstring'},
      isArray = {'bool', 'handle'},
      isMap = {'bool', 'handle'},
      isFormMap = {'bool', 'handle'},
      clear = {'void', 'handle'},

      shallowCopy = {'handle', 'handle'},
      deepCopy = {'handle', 'handle'},
    }
  )

local JArrayNativeFuncs = retrieveNativeFunctions('JArray',
    {
      object = {'handle'},
      objectWithSize = {'handle', 'uint32_t'},
      --getInt = {'int32_t', 'handle, index, int32_t'},
      --getFlt = {'float', 'handle, index, float'},
      --setInt = {'void', 'handle, index, int32_t'},
      --setFlt = {'void', 'handle, index, float'},
      --valueType = {'int32_t', 'handle, index'},
    }
  )
  
local JMapNativeFuncs = retrieveNativeFunctions('JMap',
    {
      object = {'handle'},
      removeKey = {'bool', 'handle, cstring'},
      allKeys = {'handle', 'handle'},
      allValues = {'handle', 'handle'},
    }
  )

local JFormMapNativeFuncs = retrieveNativeFunctions('JFormMap',
    {
      object = {'handle'},
      removeKey = {'bool', 'handle, CForm'},
      allKeys = {'handle', 'handle'},
      allValues = {'handle', 'handle'},
    }
  )
----------------------------------------

-- Tables

local JValue = {}
local JArray = {}
local JMap = {}
local JFormMap = {}

---------------------------------------
-- JArray is 1, JMap 2, JFormMap 3
local JCTypeList = {JArray, JMap, JFormMap}

JArray.typeName = 'JArray'
JMap.typeName = 'JMap'
JFormMap.typeName = 'JFormMap'

-------------------------------------

local CArray = ffi.typeof('struct { void* ___id; }')
local CMap = ffi.typeof('struct { void* ___id; }')
local CFormMap = ffi.typeof('struct { void* ___id; }')

local CTypeList = {CArray, CMap, CFormMap}

--------------------------------------
local JCObject_common_properties = {

  isEqualToTypeOf = function(meta, optr) return meta == JCTypeList[jclib.JValue_typeId(optr.___id)] end,
  typeOf = function(optr) return JCTypeList[jclib.JValue_typeId(optr.___id)] end,

  __gc = function(optr) jclib.JValue_release(optr.___id) end,
  __len = function(optr) return JValueNativeFuncs.count(optr.___id) end,
  __eq = function(l, r) return l.___id == r.___id end,
}

--------------------------------------

-- Primitive types

local Handle = ffi.typeof('void *')

-- Carries data from C++ to Lua
local JCToLuaValue = ffi.typeof('JCToLuaValue')

-- Carries string from C++ to Lua
local CString = ffi.typeof('CString')

-- Carries value from Lua to C++
local JCValue = ffi.typeof('JCValue')

local CForm = ffi.metatype('CForm', { __eq = function(l, r) return l.___id == r.___id end })

-------------------------------------

-- wraps and retains a jc-object handle
local function wrapJCHandle(handle)
  if handle == nil then return nil end
  
  local ctype = CTypeList[jclib.JValue_typeId(handle)]
  assert(ctype, 'invalid typeid?')

  jclib.JValue_retain(handle)
  return ctype(handle)
end

local function wrapJCHandleAsNumber(number)
  return wrapJCHandle(ffi.cast(Handle, number))
end

local JCValueType = {
  no_item = 0,
  none = 1,
  integer = 2,
  real = 3,
  form = 4,
  object = 5,
  string = 6,
}

-- Converts and returns JCToLuaValue as a lua type (string, number) or as a CForm, JCObject
-- Also frees JCToLuaValue's underlying string
local function returnLuaValue(item)
  local tp = item.type
  local v

  if item == nil or tp == JCValueType.no_item or tp == JCValueType.none then
    --v = nil
  elseif tp == JCValueType.string then
    v = ffi.string(item.string, item.stringLength)
    jclib.JCToLuaValue_free(item)
  elseif tp == JCValueType.integer then
    v = item.integer
  elseif tp == JCValueType.real then
    v = item.real
  elseif tp == JCValueType.form then
    v = item.form
  elseif item.type == JCValueType.object then
    v = wrapJCHandle(item.object)
  end
  --print('returnLuaValue result', v)
  return v
end

-- Converts Lua variable into 'JCValue'
-- I'm afraid this may turn into the most performance expensive part
local function returnJCValue(luaVar)
  local tp = type(luaVar)
  local v

  local function makeJCValue(tp, value)
    local var = JCValue(JCValueType[tp])
    var[tp] = value
    return var
  end
  
  if tp == 'number' then
    v = makeJCValue('real', luaVar)
  elseif tp == 'string' then
    v = makeJCValue('string', luaVar)
  elseif tp == 'boolean' then
    v = makeJCValue('integer', luaVar == true)
  elseif ffi.istype(CForm, luaVar) then
    v = makeJCValue('form', luaVar)
  elseif ffi.istype(CArray, luaVar) or ffi.istype(CMap, luaVar) or ffi.istype(CFormMap, luaVar) then
    v = makeJCValue('object', luaVar.___id)
  end
  
  return v
end

-- Mix common propeties into metatables
do

  local function addCommonProperties(jc_object_metatable)
    for k, v in pairs(JCObject_common_properties) do
      assert(jc_object_metatable[k] == nil)
      jc_object_metatable[k] = v
    end
  end

  addCommonProperties(JValue)
  addCommonProperties(JArray)
  addCommonProperties(JMap)
  addCommonProperties(JFormMap)
end

-- JValue
function JValue.readFromFile (path)
  return wrapJCHandle(JValueNativeFuncs.readFromFile(path))
end

function JValue.writeToFile  ( optr, path )
  JValueNativeFuncs.writeToFile(optr.___id, path)
end

function JValue.clear (optr)
  JValueNativeFuncs.clear(optr.___id)
end

function JValue.shallowCopy (optr)
  return wrapJCHandle(JValueNativeFuncs.shallowCopy(optr.___id))
end

function JValue.deepCopy (optr)
  return wrapJCHandle(JValueNativeFuncs.deepCopy(optr.___id))
end

-- JArray
do
  -- converts 1-based positive indexes to 0-based, doesn't change negative ones
  local function convertIndex(idx) return idx > 0 and idx - 1 or idx end 

  function JArray.object()
    return wrapJCHandle(JArrayNativeFuncs.object())
  end

  function JArray.objectWithSize(size)
    return wrapJCHandle(JArrayNativeFuncs.objectWithSize(size))
  end

  function JArray.objectWithArray (array)
    local object = JArray.objectWithSize(#array)
    for i,v in ipairs(array) do
      object[i] = v
    end
    return object
  end

  function JArray.insert(optr, value, idx)
    jclib.JArray_insert(optr.___id, returnJCValue(value), convertIndex(idx or -1))
  end

  function JArray.__index (optr, idx)
    return returnLuaValue(jclib.JArray_getValue(optr.___id, convertIndex(idx)))
  end

  function JArray.__newindex (optr, idx, value)
    jclib.JArray_setValue(optr.___id, convertIndex(idx), returnJCValue(value))
  end

  function JArray.__ipairs (optr)
    local iterator = function(optr, idx)
      idx = idx + 1
      if idx <= #optr then
        return idx, optr[idx]
      end
    end
    
    return iterator, optr, 0
  end

  JArray.__pairs = JArray.__ipairs
end
--------------------------------------
-- JMap stuff
do
  function JMap.object (optr)
    return wrapJCHandle(JMapNativeFuncs.object())
  end

  function JMap.objectWithTable (t)
    local object = JMap.object()
    for k,v in pairs(t) do
      object[k] = v
    end
    return object
  end

  function JMap.__index (optr, key)
    return returnLuaValue(jclib.JMap_getValue(optr.___id, key))
  end

  function JMap.__newindex (optr, key, value)
    if value then
      jclib.JMap_setValue(optr.___id, key, returnJCValue(value))
    else
      JMapNativeFuncs.removeKey(optr.___id, key)
    end
  end

  function JMap.__pairs (optr)
    local function iterator (optr, key)
      -- nextKey is CString cdata which must be freed
      local nextKey = jclib.JMap_nextKey(optr.___id, key)
      if nextKey.str ~= nil then
        local luaString = ffi.string(nextKey.str, nextKey.length)
        jclib.CString_free(nextKey)
        return luaString, optr[luaString]
      end
    end

    return iterator, optr, nil
  end

  function JMap.allKeys(optr)
    return wrapJCHandle(JMapNativeFuncs.allKeys(optr.___id))
  end

  function JMap.allValues(optr)
    return wrapJCHandle(JMapNativeFuncs.allValues(optr.___id))
  end
end
---------------------------------------
-- FormMap stuff
do
  function JFormMap.object (optr)
    return wrapJCHandle(JFormMapNativeFuncs.object())
  end

  function JFormMap.objectWithTable (table)
    local object = JFormMap.object()
    for k,v in pairs(table) do
      object[k] = v
    end
    return object
  end

  function JFormMap.__index (optr, key)
    return returnLuaValue(jclib.JFormMap_getValue(optr.___id, key))
  end

  function JFormMap.__newindex (optr, key, value)
    if value ~= 0 then
      jclib.JFormMap_setValue(optr.___id, key, returnJCValue(value))
    else
      JFormMapNativeFuncs.removeKey(optr.___id, key)
    end
  end

  function JFormMap.__pairs (optr)
    local function iterator (optr, key)
      local nextKey = jclib.JFormMap_nextKey(optr.___id, key)
      if nextKey.___id ~= 0 then
        return nextKey, optr[nextKey]
      end
    end

    return iterator, optr, CForm(0)
  end

  function JFormMap.allKeys(optr)
    return wrapJCHandle(JFormMapNativeFuncs.allKeys(optr.___id))
  end

  function JFormMap.allValues(optr)
    return wrapJCHandle(JFormMapNativeFuncs.allValues(optr.___id))
  end
end
--------------------------------------- 

-- Associate CTypes with metatables
do
  assert(#CTypeList == #JCTypeList)
  for i, ctype in ipairs(CTypeList) do
    local metatable = JCTypeList[i]
    readonlytable(metatable)
    ffi.metatype(ctype, metatable)
  end
end

-- TESTS
local function testJC()
  
  do
    assert(CForm(1) == CForm(1))
    assert(CForm(1) ~= CForm(2))
  end
  
  local function expectSize(o, size)
    assert(o)
    local s = JValue.typeOf(o).typeName
    assert(#o == size, 'expected object size is '..size..', got '..#o..' . type: '..s)
  end

  local function makeRandomContainerOfType(jtype)

    local function randomString()
      local len = math.random(1, 10)
      local s = ''
      for i=1,len do
        s = s .. string.char(math.random(32, 95))
      end
      return s
    end

    local function randomNumber()
      return math.random(-50.5, 50.5)
    end

    local function randomForm()
      return CForm(math.random(0xff000001, 0xfffffffe))
    end

    local function randomObject()
      return JCTypeList[math.random(1, #JCTypeList)].object()
    end

    local random_funcs = {randomString, randomNumber, randomObject, randomForm}

    local function randomValue()
      return random_funcs[math.random(1, #random_funcs)]
    end

    local type2Func = {}

    type2Func[JArray] = function ( )
      local len = math.random(0, 10)
      local object = JArray.objectWithSize(len)
      for i=1,len do
        object[i] = randomValue()
      end
      expectSize(object, len)
      return object
    end
    
    local function expectSize2(o, size)
      local s = JValue.typeOf(o).typeName
      assert(size == 0 or #o > 0, s)
    end

    type2Func[JMap] = function ( )
      local len = math.random(0, 10)
      local object = JMap.object()
      for i=1,len do
        object[randomString()] = randomValue()
      end
      expectSize2(object, len)
      return object
    end

    type2Func[JFormMap] = function ( )
      local len = math.random(0, 10)
      local object = JFormMap.object()
      for i=1,len do
        object[randomForm()] = randomValue()
      end
      expectSize2(object, len)
      return object
    end

    return type2Func[jtype]()
  end

  local function eqTest( o1, o2 )
    assert(o1 ~= o2)
    assert(o1 == o1)
    assert(o2 == o2)

    local jtype1 = JValue.typeOf(o1)
    local jtype2 = JValue.typeOf(o2)
    assert(jtype1 == jtype2)

    assert(jtype1:isEqualToTypeOf(o1))
    assert(jtype2:isEqualToTypeOf(o2))
  end

  local function hasEqualContent(o, o2)
    local otype = JValue.typeOf(o)
    if otype ~= JValue.typeOf(o2) then return false end

    for k,v in pairs(o) do
      if o2[k] ~= v then return false end
    end

    return true
  end

  local function readWriteJSONTest( o )
    assert(o)

    local path = 'C:/lua/i_love_lua2.txt'
    JValue.writeToFile(o, path)

    local o2 = JValue.readFromFile(path)
    assert(o2)
    assert(o ~= o2)
    assert(JValue.typeOf(o) == JValue.typeOf(o2), 'types are not equal. '..JValue.typeOf(o).typeName..'!='..JValue.typeOf(o2).typeName)
    assert(#o == #o2, 'lengths ' .. #o .. ' ~= ' .. #o2)

    assert(hasEqualContent(o, o2))
  end

  local function testType(jtype)
    print('testType: ', jtype.typeName)
    
    local o = makeRandomContainerOfType(jtype)

    eqTest(o, makeRandomContainerOfType(jtype))
    readWriteJSONTest(o)

    JValue.clear(o)
    assert(#o == 0)
  end

  -- test randomly created objects
  for i=1,20 do
    for _, jtype in ipairs(JCTypeList) do
      testType(jtype)
    end
  end
  --[[
  local function doWithTiming(operation_name, operation)
    local x = os.clock()
    operation()
    print(string.format("operation %s takes: %.2f", operation_name, os.clock() - x))
  end
  
  do
    -- ROUND 1
    local jstrings = JValue.readFromFile('C:/lua/strings.json')
    assert(jstrings and #jstrings == 128)

    -- move content into table strings to not interfer with JC
    local strings = {}
    for _, v in pairs(jstrings) do
      strings[#strings + 1] = v
    end

    doWithTiming('dumb concatenation', function()
        local s = ''
        for _, v inpairs(strings) do
          s = s + v
        end
        print('concatenated string: ' .. s)
      end)


    doWithTiming('smart concatenation', function()
        local s = {}
        for _, v inpairs(strings) do
          s[#s + 1] = v
        end
        print('concatenated string: ' .. table.concat(s))
      end)

  end
  --]]
end

--testJC()
--collectgarbage('collect')

return {

  public = {
      JValue = JValue,
      JArray = JArray,
      JMap = JMap,
      JFormMap = JFormMap,
      JDB = wrapJCHandle(jclib.JDB_instance(JConstants.Context)),

      Form = CForm,
  },

  testJC = testJC,
  wrapJCHandle = wrapJCHandle,
  returnJCValue = returnJCValue,
  wrapJCHandleAsNumber = wrapJCHandleAsNumber,
}

