
local JCTypeList = {JArray, JMap, JFormMap}

local function print( ... )
end

local basicTest = function()
  
  do
    assert(Form(1) == Form(1))
    assert(Form(1) ~= Form(2))
  end
  
  local function expectSize(o, size)
    assert(o)
    local s = JValue.typeOf(o).typeName
    assert(#o == size, 'expected object size is '..size..', got '..#o..' . type: '..s)
  end

  local function makeRandomContainerOfType(jtype, ...)
    local op_table = ...

    local function getOptOrDef(key, default)
      return (op_table and op_table[key] ~= nil) and op_table[key] or default
    end

    local len = getOptOrDef('minLength',0) + math.random(1, 10)

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
      return Form(math.random(0xff000001, 0xfffffffe))
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
      local object = JArray.objectWithSize(len)
      for i=1,len do
        object[i] = randomValue()
      end
      expectSize(object, len)
      return object
    end
    
    local function expectSize2(o, size)
      local s = JValue.typeOf(o).typeName
      assert(size == 0 or #o > 0, string.format('the size of container of type %s should be 0 or %u', s, #o))
    end

    type2Func[JMap] = function ( )
      local object = JMap.object()
      for i=1,len do
        object[randomString()] = randomValue()
      end
      expectSize2(object, len)
      return object
    end

    type2Func[JFormMap] = function ( )
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

    print('WARNING - readWriteJSONTest disabled')
    do return end

    local path = os.tmpname()
	do
      JValue.writeToFile(o, path)

      local o2 = JValue.readFromFile(path)
      assert(o2, "can't read from "..path)
      assert(o ~= o2)
      assert(JValue.typeOf(o) == JValue.typeOf(o2), 'types are not equal. '..JValue.typeOf(o).typeName..'!='..JValue.typeOf(o2).typeName)
      assert(#o == #o2, 'lengths ' .. #o .. ' ~= ' .. #o2)

      assert(hasEqualContent(o, o2))
	end
	os.remove (path)
  end

  local function testType(jtype)
    print('testType: ', jtype.typeName)
    
    local o = makeRandomContainerOfType(jtype)

    eqTest(o, makeRandomContainerOfType(jtype))
    readWriteJSONTest(o)

    JValue.clear(o)
    assert(#o == 0)
  end


  local function testMapType(jtype)
    print('testMapType: ', jtype.typeName)
    
    do
      local o = makeRandomContainerOfType(jtype, {minLength = 1})
      assert(#o > 0)
      local len = #o
      local any_key = jtype.allKeys(o)[math.random(1, #o)]
      assert(any_key, "nil keys is not ok")
      o[any_key] = nil
      assert(#o + 1 == len, "length not decreased by 1")
    end

  end

  -- test randomly created objects
  for i=1,20 do
    for _, jtype in ipairs(JCTypeList) do
      testType(jtype)
    end
  end

  for i=1,20 do
    for _, jtype in ipairs({JMap, JFormMap}) do
      testMapType(jtype)
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

return {basicTest = basicTest}
