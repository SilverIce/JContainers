
local jc = jrequire 'jc'
local misc = jrequire 'testing.misc'

return function()

  misc.performTest('accumulateValues', function()
    local obj = JArray.objectWithArray({1,1,2,3,5,8,13})
    assert(obj ~= nil)
    assert(jc.accumulateValues(obj, function(a,b) return a+b end) == 33)
  end)

  misc.performTest('accumulateKeys', function()
    local obj = JArray.objectWithArray({1,1,2,3,5,8,13})
    assert(obj ~= nil)
    assert(jc.accumulateKeys(obj, math.max) == #obj)
  end)

end
