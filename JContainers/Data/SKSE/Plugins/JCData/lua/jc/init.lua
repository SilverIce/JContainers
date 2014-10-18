
--[[

For testing purposes only!

You may add more functionality via creating a new .lua file in JCData/lua/ directory

Example 1

    JValue.evalLuaFlt( 0, "return math.pi" ) is 3.14156...

Example 2

    int obj = JValue.objectFromPrototype("[1,2,3,4,5,6]")
    JValue.evalLuaInt( obj, "return jc.count(jobject, jc.less(6))" ) ; should return 5

Example 3 (pseudo-code)

    obj = [
            {   "theSearchString": "a",
                "theSearchForm" : "__formData|A|0x14"
            },

            {   "theSearchString": "b",
                "theSearchForm" : "__formData|A|0x15"
            }
        ]
    
    JValue.evalLuaInt(obj, "return jc.find(jobject, function(x) return x.theSearchString == 'b' end") is 1

--]]

print("JC module loaded!")

local jc = {}


-- function which iterates over collection's elements
-- @predicate parameter is a function which accepts collection's item, returns true to stop iteration
function jc.apply(collection, predicate)
    for i,v in pairs(collection) do
        if predicate(v) then return end
    end

    JDB.AproposForms[actor].health = 10
    -- or

    -- local aprActrs = JDB.AproposForms

    -- aprActrs[actorA].health = 10
    -- aprActrs[actorA].friends = JArray.objectWithTable {friendA, friendB}
    
end

-- function filters collection, returns new JArray colletion containing filtered values
-- @predicate parameter is a function which accepts collection's item, returns true if item satisfying predicate
function jc.filter(collection, predicate)
    local array = JArray.object()
    for k,v in pairs(collection) do
        if predicate(v) then JArray.insert(array, v) end
    end

    return array
end


-- Lua functions:

-- returns amount of items in collection satisfying predicate
function jc.count(collection, predicate)
    local matchCnt = 0

    for _,v in pairs(collection) do
        if predicate(v) then matchCnt = matchCnt + 1 end
    end

    return matchCnt
end

-- returns first index/key of item in collection satisfying predicate
function jc.find(collection, predicate)
    for k,v in pairs(collection) do
        if predicate(v) then return k end
    end
end


-- Various predicates:

function jc.less(than)
    return function(x) return x < than end
end

function jc.greater(than)
    return function(x) return x > than end
end

return jc
