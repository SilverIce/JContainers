
--[[

Do not modify this file!
You may add more functionality via creating a new init.lua file in 'JCData/lua/Mod-Specific-Folder' directory

Example 1

    JValue.evalLuaFlt( 0, "return math.pi" ) is 3.14156...

Example 2

    int obj = JValue.objectFromPrototype("[5,3,1,4,5,6]")
    -- Counts an items where values are lesser than 6. Returns 5
    JValue.evalLuaInt( obj, "return jc.count(jobject, jc.less(6))" )

Example 3 (pseudo-code)

    obj = [
            {   "theSearchString": "a",
                "theSearchForm" : "__formData|A|0x14"
            },

            {   "theSearchString": "b",
                "theSearchForm" : "__formData|A|0x15"
            }
        ]
    
    -- returns an index where x.theSearchString == 'b'. Since Lua indexing is one-based, 'evalLuaInt' returns 2
    JValue.evalLuaInt(obj, "return jc.find(jobject, function(x) return x.theSearchString == 'b' end")

--]]

local jc = {}


-- function which iterates over collection's elements
-- @predicate parameter is a function which accepts collection's item, returns true to stop iteration
function jc.apply(collection, predicate)
    for i,v in pairs(collection) do
        if predicate(v) then return end
    end
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

-- returns first index or key of item in collection satisfying predicate
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
