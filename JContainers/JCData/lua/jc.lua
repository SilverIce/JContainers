
--[[

For testing purposes only!

You may add more functionality via creating a new .lua file in JCData/lua/ directory

Example 1

    JValue.evalLuaFlt( 0, "return math.pi" ) is 3.14156...

Example 2

    int obj = JValue.objectFromPrototype("[1,2,3,4,5,6]")
    JValue.evalLuaInt( obj, "return count(jobject, less(6))" ) ; should return 5

Example 3 (pseudo-code)

    obj = [
            {   "theSearchString": "a",
                "theSearchForm" : "__formData|A|0x14"
            },

            {   "theSearchString": "b",
                "theSearchForm" : "__formData|A|0x15"
            }
        ]
    
    JValue.evalLuaInt(obj, "return find(jobject, function(x) return x.theSearchString == 'b' end") is 1

--]]


-- global JC container object (collection) on whom operation will be performed
jobject = nil


-- Native functions:

-- apply is a native function, which iterates through collection elements
-- lambda parameter is a function which accepts collection's item, returns true to stop iteration
-- function apply(collection, lambda)

-- function filters collection, returns new JArray colletion containing filtered values
-- lambda parameter is a function which accepts collection's item, returns true i
-- function apply(collection, lambda)


-- Lua functions:

-- returns amount of items in collection satisfying predicate
function count(collection, predicate)
    local matchCnt = 0

    apply(collection,
        function(x)
            if predicate(x) then
                matchCnt = matchCnt + 1
            end

            return false
        end
    )
    return matchCnt
end

function find(collection, predicate)
    local index = -1
    local foundIndex = -1

    apply(collection,
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

function less(than)
    return function(x) return x < than end
end

function greater(than)
    return function(x) return x > than end
end
