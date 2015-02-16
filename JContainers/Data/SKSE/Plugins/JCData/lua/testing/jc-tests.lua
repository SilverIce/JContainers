
local jc = jrequire 'jc'
local misc = jrequire 'testing.misc'

return function()

  return misc.performTest('accumulateValues', function()
    local obj = JArray.objectWithArray({1,1,2,3,5,8,13})
    assert(obj ~= nil)
    assert(jc.accumulateValues(obj, function(a,b) return a+b end) == 33)
  end)

  and misc.performTest('accumulateKeys', function()
    local obj = JArray.objectWithArray({1,1,2,3,5,8,13})
    assert(obj ~= nil)
    assert(jc.accumulateKeys(obj, math.max) == #obj)
  end)


  and misc.performTest('accumulateValues + pathResolving', function()

    assert( 2 == JValue.solvePath(JValue.objectFromPrototype('[2]'), '[0]') )

    local obj = JValue.objectFromPrototype [[
      [
        {"magnitude": -9},
        {"magnitude": 11},
        {"magnitude": 3}
      ]
    ]]

    assert(obj ~= nil)
    assert(#obj == 3)
    assert(obj[1].magnitude == -9)

    assert(jc.accumulateValues(obj, math.max, '.magnitude') == 11)
    assert(jc.accumulateValues(obj, function(a,b) return a + b end, '.magnitude') == 5)
  end)

end
