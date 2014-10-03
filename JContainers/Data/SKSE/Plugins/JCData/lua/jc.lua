
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


-- global JC container object (collection) on whom operation will be performed
jobject = nil

-- namespace
jc = {}

-- Native functions (placeholders):

-- apply is a native function, which iterates through collection elements
-- @predicate parameter is a function which accepts collection's item, returns true to stop iteration
function jc.apply(collection, predicate) end

-- function filters collection, returns new JArray colletion containing filtered values
-- @predicate parameter is a function which accepts collection's item, returns true if item satisfying predicate
function jc.filter(collection, predicate) end


-- Lua functions:

-- returns amount of items in collection satisfying predicate
function jc.count(collection, predicate)
    local matchCnt = 0

    jc.apply(collection,
        function(x)
            if predicate(x) then
                matchCnt = matchCnt + 1
            end

            return false
        end
    )
    return matchCnt
end

-- returns first index of item in collection satisfying predicate
function jc.find(collection, predicate)
    local index = -1
    local foundIndex = -1

    jc.apply(collection,
        function(x)
            index = index + 1
            if predicate(x) then
                foundIndex = index
                return true
            else
                return false
            end
        end
    )
    return foundIndex
end


-- Various predicates:

function jc.less(than)
    return function(x) return x < than end
end

function jc.greater(than)
    return function(x) return x > than end
end

return jc
