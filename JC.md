
## Motivation & Overview

Papyrus lacks convenient data structures such as dynamic arrays or associative containers. Scripting languages such as Papyrus should be simple but not underdeveloped.

This plugin attempts to add missing functionality. Although it is not native functionality and it will never have the nice brackets to access items that the default Papyrus Array has, I believe it is still better than nothing.

The current version implements array and associative (map or dictionary) containers: JArray (array container), JMap and JFormMap (both associative containers) and a few convenient wrappers: JDB and JFormDB (databases).

## Table of Contents

* [Reference](JC.md#reference)
* [Tutorial](JC.md#tutorial)

## Reference

#### What is a value?

Containers are intended to contain values. A value is a float, integer, string, form or another container. 

#### Basics. Object (container) instantiation, identifiers

To use any container object (array, map or form-map), you must first create (instantiate) it with the `object` function or retrieve it from somewhere:

```lua
int array = JArray.object()
int anotherArray = JDB.solveObj(".myArray")
int map = JMap.object()
```
Any function that returns an 'object' actually returns its **identifier**. An identifier is a unique number that ranges from 1 to 2^32. It's how it is distinguished from other objects, and almost the only way to interact with it.

Once created, you may put data in the array:
```lua
JArray.addStr(array, "it’s me")
JArray.addForm(array, GetTargetActor())
```
And read the array's contents:        
```lua
string text = JArray.getStr(array, 0)
form actor = JArray.getForm(array, 1)
```

#### JArray

Ordered collection (array) of values. It is dynamically resizeable, and can store any number of values of any combination of types (default Papyrus Arrays are limited at 128 items).

#### JMap and JFormMap

Both are **associative** containers (sets of unique keys and values where each key associated with one **value**). Each key must be unique within a given container. In a JMap, a key is a string, while a JFormMap key is a form (a form is any actor, item, quest, spell - almost everything in Skyrim).
```lua
int map = JMap.object()
JMap.setForm(map, "me", GetTargetActor())
form actor = JMap.getForm(map, "me")
```

#### JValue

Nothing more that just an interface that shows what functionality JArray, JMap and JFormMap share. So each of them can be emptied, serialized and deserialized to/from JSON and more.

```lua
int array = JArray.object()
int map = JMap.object()
-- equivalent ways to do same things.
-- all count functions return zero as new containers are empty
JValue.count(array) == JArray.count(array) == JValue.count(map)
-- write container content into file:
JValue.writeToFile(map, "map.txt")
JValue.writeToFile(array, "array.txt")
```

#### JDB

Take it as a global entry point or database - you put information in it under a string key "yourKey", and then you can access to it from any script in the game. There is only one JDB in game, so each time you access it you access that one, single JDB. It is an **associative** container like JMap (it is, in fact, JMap internally), but the script interface is slightly different.

Typical JDB usage would involve:

1. Setup (during mod installation) where you specify `root key` name. In example below root key is `frostfall`.

 > **Important**
 > Choose root name carefully to avoid clashes with rest of JDB root keys and with JFormDB storage names.

2. Access data:

    ```lua
    -- 1. Setup procedure
    int frosfallData = JValue.readFromFile("frostfall_config.json")
    JDB.setObj("frostfall", frosfallData)

    -- 2. read/write data later in another script:
    int lighttignMode = JDB.solveInt(".frostfall.campfileLightingMode")
    JDB.solveIntSetter(".frostfall.campfileLightingMode", 1)
    ```



#### JFormDB

Provides a convenient way to associate values with a form. You may find it looking like a mix of JMap and JDB - like JDB, there is only one JFormDB in game, and it's associative container, like JMap. Also it supports path resolving.
To store or retrieve value form-key and string path must be passed:

```lua
-- store...
form me
JFormDB.setFlt(me, ".yourModFormStorage.valueKey", 10)
JFormDB.setStr(me, ".yourModFormStorage.anotherValueKey", "name")
-- and retrieve values
float value = JFormDB.getFlt(me, ".yourModFormStorage.valueKey")
```

String path must always consist of two parts: `formStorageName` and `valueKey`

`valueKey` is a key used to retrieve value or create {valueKey, value} association for a form.

`formStorageName` is a JFormMap containing {formKey, {valueKey, value}} associations.
It was added to avoid possible collisions: one mod may occasionally override value written by another mod if I'd allowed simple paths without storage name part. The fact that it is a separate storage makes it possible to access that storage, delete it without any risk to delete another mod data:

```lua
-- Will destroy everything "yourModFormStorage" contains
JDB.setObj("formStorageName", 0)
```

How any `set*` function works internally:
Once a value gets assigned via `JFormDB.set*(formKey, ".formStorageName.valueKey", value)`, JFormDB looks for `formStorageName` in the JDB (or creates it if it isn't found) and then looks for the JMap entry associated with the form key (or it creates an entry if none is found) and then creates a {valueKey, value} pair.

Slightly more advanced usage:

```lua
-- Will destroy everything associated with 'me' form in "yourModFormStorage" storage
JFormDB.setEntry("yourModFormStorage", me, 0)
 
-- Custom entry type
JFormDB.setEntry("yourModFormStorage", me, JArray.object())
```

### API usage notes

First and foremost, the game will not crash no matter what data you pass into JContainer functions. The following happens if a function gets called with invalid input (when input cannot be handled properly):

All functions returning new containers return zero identifier. For. ex `JValue.readFromFile("")` returns 0 because of an invalid file path. Zero identifier means non-existing object. It’s ok to pass it into other functions - in that case the function will return the default value.

All functions that read container contents (such as `getFlt`, `solveFlt`, `getStr`, `count`, `allKeys`, etc.) return the default value. For function that return an integer or float, the default value is 0, for functions that return a string or form the default value is `None`, and for functions that return a container the default value is 0.

### Object persistence

Every container object persists in save file until the container gets destroyed. When a save is performed, all objects are saved and all objects are resurrected when the save file gets loaded.

### JSON serialization

As said above, it's possible to serialize/deserialize container data (write to or read from an external file). While numbers and strings are serialized in a natural way, storing form information is slightly tricky because JSON knows nothing about Skyrim forms (and also because global form id depends on mod load order). A serialized form is a string prefixed with `"__formData"`, the plugin file name, and the local or global form id (hex or decimal number).

Serialization:
```lua
int playerData = JMap.object()
JMap.setForm(playerData, "actor", playerForm)
JMap.setInt(playerData, "level", playerForm.GetLevel())
JValue.writeToFile(playerData, "Data/playerInfo.txt")
```

Example of serialized JMap containing player's form associated with `"test"` key:
```json
{
    "test": "__formData|Skyrim.esm|0x14",
    "name": "Elsa",
    "level": 2
}
```
Example of a serialized array woth a nested form-map container:
```json
[
    0,
    1.5,
    {
        "__formData": null,
        "__formData|Skyrim.esm|0xc0ffee" : "coffee",
        "__formData|Dawnguard.esm|0xc0a1bd" : 2.5,
    },
    "just a string"
]
```

Deserialization:
```lua
int data = JValue.readFromFile("Data/playerInfo.txt")
int level = JValue.solveInt(data, ".level")
form player = JValue.solveForm(data, ".actor")
```

### Path resolving

This feature simplifies an access to values of nested objects via group of `solve*` and `solve*Setter` functions. Each function takes path specifier, which determines in which key to search for a value. For example:

```lua
solveInt(objectA, ".keyA[4].keyB")
```
retrieves a value which is associated with keyB of JMap, which located at 4-th index of JArray, which is associated with keyA of objectA-JMap. Huh.

`solve*Setter` changes (assigns) a value. Also there is an optional `createMissingKeys` argument - if enabled, will insert any missing JMap key during path traversal. For example, calling `solveFltSetter(objectA, ".key1.key2", 3.14, true)` on an empty objectA will create new JMap B containing `{"key2", 3.14}` pair and associate objectA with new JMap B (i.e. `{"key1", {"key2": 3.14}}` structure will be created). `solve*Setter` fails if `createMissingKeys` is disabled and any key in the path is missing.

More examples:

```lua
info = {
    "classicPreset" :  {
        "campfileLighting" : "Automatic"
    },
    "numbers" : [0, 1, 2, 3]
}

string lightingType = JValue.solveStr(info, ".classicPreset.campfileLighting")
JValue.solveStrSetter(info, ".classicPreset.campfileLighting", "Non-Automatic")
int firstNumber = JValue.solveInt(info, ".numbers[0]")
JValue.solveIntSetter(info, ".numbers[0]", 10)
```

### Collection operators

>**Important**
> Collection operators is deprecated feature and will be replaced with [Lua](JC.md#lua)

This feature allows executing functions on collection (container) elements. It's accessible via solve* functions.
Syntax:

* @function
* @function.path.to.element
* path.to.container@function
* path.to.container@function.path.to.element

path.to.container - the path to the collection you want to retrieve.

function - the function that will be applied on each element of the collection. Currently these functions are implemented:

* minNum, maxNum (search for min or max number, works with any number type (int or float))
* minFlt, maxFlt - the same as above, but accepts float values only
* minInt, maxInt - the same as above, but accepts integer values only

path.to.element - the path to the element you want to retrieve.

Examples (pseudo-code):
```lua
obj = [1,2,3,4,5,6]

solveFlt(obj, "@maxNum") is 6
solveFlt(obj, "@minNum") is 1

obj = { "a": [1], "b": {"k": -100}, "c": [3], "d": {"k": 100}, "e": [5], "f": [6] }

solveFlt(obj, "@maxNum.value[0]") is 6
solveFlt(obj, "@minNum.value[0]") is 1
solveFlt(obj, "@maxNum.value.k") is 100
solveFlt(obj, "@minNum.value.k") is -100

obj = {
    "mapKey": { "a": [1], "b": {"k": -100}, "c": [3], "d": {"k": 100}, "e": [5], "f": [6] }
}

solveFlt(obj, ".mapKey@maxNum.value.k") is 100
```

### Key naming convention

In order to make path resolving and collection operators function properly, string keys should consist of ASCII characters and should not contain the decimal character, square brackets, or the `@` character. For instance, the following code will fail to work:
```lua
obj = { "invalid.key" : {"k": 10} }

solveInt(map, ".invalid.key.k") is 0

-- although it's still possible to access that value in the traditional way:
getObj(map, "invalid.key") is {"k": 10}
```
This convention applies to every key string, not just the JMap key.  It affects JFormDB storage name and keys as well as JDB.setObj key. Key naming shouldn't matter if you don't use path resolving.

### Number conversion notes

Functions that handle numbers (`getFlt`, `solveFlt`, `getInt`, `solveInt`) will convert the numbers they handle into their respective types.  For example, `getFlt` will return a float `1.0` if the number passed to it is the int `1`.  On the other hand, the rest of the `get*` and `solve*` functions may fail to perform conversions and will return default values.

### Lua

Since 3.0 JContainers embeds Lua. Benefits of using Lua:

- any standard lua library functionality available (bitwise operations, math, string manipulation, operating system facilities and etc)
- seek, sort (in development) JArray with user specified predicate
- move some cumbersome Papyrus code into more compact Lua (see `frostfall.uuid` function in example below)
 
> **Important**
> Lua feature status is highly experimental. It's API may change when more functionality will be added.

Typical usage may look like:

- you invoke any lua function with `JValue.evalLuaFlt/Int/Str/Form/Obj`:

 ```lua
 float pi = JValue.evalLuaFlt(0, "return math.pi")
 JValue.evalLuaInt(0, "return bit32.bxor(8, 2, 10)") -- returns 8 xor 2 xor 10
 ```
 
 ```lua
 obj = [
         {   "theSearchString": "a",
             "theSearchForm" : "__formData|A|0x14"
         },
         {   "theSearchString": "b",
             "theSearchForm" : "__formData|A|0x15"
         }
 ]
 
 -- returns 1 - an array index where `arrayItem.theSearchString == 'b'`
 JValue.evalLuaInt(obj, "return jc.find(jobject, function(x) return x.theSearchString == 'b' end")
 ```

- you write your own functionality in a _Data/SKSE/Plugins/JCData/lua/frostfall.lua_ file:

 ```lua
 -- frostfall module depends on jc.count function from 'JCData/lua/jc.lua'
 require 'jc'
 
 frostfall = {}
 
 function frostfall.countItemsLessAndGreaterThan(collection, less, greater)
     return jc.count(collection, function(x)
        return x < less and x > greater
     end)
 end
 
 -- generates random guid-string (may return 'd6cce35c-487a-458f-bab2-9032c2621f38' once per billion years)
 function frostfall.uuid()
    local random = math.random
    local template ='xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'
    return string.gsub(template, '[xy]', function (c)
        local v = (c == 'x') and random(0, 0xf) or random(8, 0xb)
        return string.format('%x', v)
    end)
 end

 return frostfall
 ```
 Papyrus:
 ```lua
 
  JValue.evalLuaInt(obj, "return frostfall.countItemsLessAndGreaterThan(jobject, 60, -5)")
  string guid = JValue.evalLuaStr(0, "return frostfall.uuid()")
 ```
 
### Object lifetime management rules

Each time a script creates a new string or Papyrus array, Skyrim allocates memory and automatically frees it when you do not need that string or array anymore.

In JContainers, internally all containers are C++ objects, so Skyrim knows nothing about them and can not manage their lifetime and memory.

The lifetime management model is based on object ownership. Any container object may have one or more owners. As long as an object has at least one owner, it continues to exist. If an object has no owners it gets destroyed.

Functionality to manage object's lifetime:

- Retain, release functions:

 ```lua
int function retain(int object, string tag="")
int function release(int object)
function releaseObjectsWithTag(string tag)
 ```
 The lifetime model implemented using simple owner (reference) counting. Each object have a such counter. Each time the object gets inserted into another container or `JValue.retain` is called the counter increases by 1. Each time the object gets removed from a container or released via `JValue.release` the reference counter decreases by 1.
If the reference counter reaches zero, the object is temporarily owned for roughly 10 seconds. During this period of time the object have a _last chance to survive_ - and gets destroyed if nobody owns it.
Newly created objects (object created with `object`, `objectWith*`, `all/Keys/Values` or `readFromFile`) also have that _last chance to survive_.

 Illustration shows the idea: ![test][1]

 > **Important**
 > The caller of `JValue.retain` is responsible for releasing object. Not released object will remain in save file forever.
    
 `Tag` parameter marks an object. `None` tag does nothing. Must be an unique string (mod name may fit). Why we need a tag? We all human. We may forget to release an object. Papyrus may throw an error in between `retain .. release` and in a result `release` will not be executed. By tagging an object you leave a possibility to track lost objects with specific tag and release them via `JValue.releaseObjectsWithTag` function.
    
 > **Important**
 > `JValue.releaseObjectsWithTag` complements all `retain` calls with `release` that were ever made to all objects with given tag.
    
 ```lua
int function releaseAndRetain(int previousObject, int newObject, string tag=None)
 ```
 It's just a union of retain-release calls. Releases `previousObject`, retains, tags and returns `newObject`. Typical usage:
    
 ```lua
 -- create and retain an object
 self.followers = JArray.object()
 -- release object
 self.followers = 0
 -- or replace with another
 self.followers = JArray.object()

 int property followers hidden
    int function get()
        return _followers
    endFunction
    function set(int value)
        _followers = JValue.releaseAndRetain(_followers, value, "uniqueTag")
    endFunction
 endProperty
    
 int _followers = 0
 ```

- Pools: 

 ```lua
 int function addToPool(int object, string poolName) global native
 function cleanPool(string poolName) global native
 ```

 Handy for temporary objects (objects with no owners) - when it's known that object's lifetime should exceed 10 seconds. Pool `poolName` (must be an unique string - mod name may fit) owns any amount of objects, preventing their destruction, extends lifetime. Internally location is JArray - `addToPool` adds an object and `cleanPool` clears pool. Do not forget to clean location later! Typical use:

 ```lua
 int tempMap = JValue.addToPool(JMap.object(), "uniquePoolName")
 -- anywhere later:
 JValue.cleanPool("uniquePoolName")
 ```


## Tutorial

### Simple example

Suppose you want to store some actor related information (let it be player’s followers and their mood):

```lua
function storeFolloverMood(form follower, float mood)
    -- function creates "followers" storage and then creates
    -- (follower, entry) associations
    JFormDB.setFlt(follower, ".followers.mood", mood)
endfunction

float function followerMood(form follower)
    -- fetch follower mood
    return JFormDB.getFlt(follower, ".followers.mood")
endfunction

; method that gets called once user uninstalls your mod
function modUninstallMethod()
    --  destroy association to not pollute game save and precious RAM
    JDB.setObj("followers", 0)
endfunction
```
### Config reading

You wish to have all your mod config values to be stored somewhere (for ex. in `"Data/preset.txt"` file) so you could easy adjust them all by editing the file. Or you do not wish to hardcode all these values. JSON formatted file contains following information:

```json
{
    "classicPreset" : {
        "campfileLighting" : "Automatic",
        "exposureRate" : 1.0,
        "frigidWaterLethal" : 1,
        "exposureIsLethal" : 1,
        "axeDurability" : 1
    },
    "winterHorkerPreset" : {
        "campfileLighting" : "nonAutomatic",
        "exposureRate" : 0.5,
        "frigidWaterLethal" : 0,
        "exposureIsLethal" : 0,
        "axeDurability" : 0
    }
}
```
It contains root map containing two maps - two standard presets your mod provides - classicPreset & winterHorkerPreset. Config file reading may look like:

```lua
-- let it be .classicPreset or .winterHorkerPreset string
string currentPreset

-- use function each time you need re-read preset from a file
function parseConfig()
    -- that’s all. presets are already in Skyrim
    -- readFromFile returns root map container
    -- it may return zero if file not exist or it can not be parsed (not JSON format or you have accidentally added extra coma)
    int config = JValue.readFromFile("Data/preset.txt")
    -- put config into DB - associate key and config
    JDB.setObj("frostfall", config)
    currentPreset = ".classicPreset"
endfunction

bool function axeDurabilityEnabled()
    -- solveInt like any solve* function tries to find (solve) value for given path
    -- current path is ".frostfall.classicPreset.axeDurability"
    return JDB.solveInt(".frostfall" + currentPreset + ".axeDurability") != 0
endfunction

string function lightingType()
    return JDB.solveStr(".frostfall" + currentPreset + ".campfileLighting")
endfunction
```

### Config reading 2

Let it be a script that modifies model bone scales (interpolates scales between min and max) and needs configuration data to be imported from file:
```json
[
    ["NPC Head [Head]", 0, -0.33],
    ["NPC Spine [Spn0]", -0.133, -0.3],
    ["NPC Spine1 [Spn1]", 0, 0.433],
    ["NPC Spine2 [Spn2]", 0, -0.167]
]
```
What you see here is one array that contains 4 sub-arrays and each sub-array contains model bone name, minimum and maximum scale.Then script would look like:

```lua
EventOnEffectStart(Actor akTarget, Actor akCaster)
    --  read config file from game root folder and associate it with "scaleMod" key
    JDB.setObj("scaleMod", JValue.readFromFile("scale.txt"))
Endevent

function setScale(float scale)
    objectreference plr = GetTargetActor()
    
    -- retrieve config
    int config = JDB.solveObj(".scaleMod")
    
    -- iterate over array & calculate bone scale
    int i = JArray.count(config)
    while(i > 0)
        i -= 1
        -- fetch sub-array. it can be ["NPC Head [Head]", 0, -0.33] for instance
        int data = JArray.getObj(config, i)
        float nodeScale = 1.0 + JArray.getFlt(data,1) + (JArray.getFlt(data,2) - JArray.getFlt(data,1)) * scale
        NetImmerse.SetNodeScale(plr, JArray.getStr(data, 0), nodeScale, False)
    endWhile
endfunction
```

### Followers example

The same as first example, but now you need to store one more value - anger and list of victims (both are per-actor data). Also you have decided to not associate followers with JDB database.

We will store all per-actor information in following structure:
```json
{
    "mood": 0,
    "anger": 0,
    "victims": []
}
```
Here you can see a map that contains 3 key-value associations: mood and angler (both values are zeros initially) and `"victims": []` association (`[]` means empty array).
```lua
function storeFollowerMood(form follower, float mood)
    -- get follower entry to write into it
    int entry = getActorEntry(follower)
    -- write mood into follower entry
    JValue.solveFltSetter(entry, ".mood", mood)
endfunction

function addFollowerVictim(form follower, form victim)
    -- get follower entry to write into it AND then get victims array
    int victims = JValue.solveObj(getActorEntry(follower), ".victims")
    -- add victim into array
    JArray.addForm(victims, victim)
endfunction

float function followerMood(form follower)
    -- get follower entry AND fetch mood
    return JValue.solveFlt(getActorEntry(follower), ".mood")
endfunction

float function followerAnger(form follower)
    return JValue.solveFlt(getActorEntry(follower), ".anger")
endfunction

-- find (or create new if not found) per-actor information containing mood, anger and array of victims
int function getActorEntry(form actor)
    int entry = JFormMap.getObj(self.followers, follower)
    -- if no entry found - create new from prototype-string
    if !entry
        entry = JValue.objectWithPrototype("{ \"mood\": 0, \"anger\": 0, \"victims\": [] }")
        JFormMap.setObj(self.followers, follower, entry)
    endif

    return entry
endfunction

-- property hides all black magick - retains & releases object
-- see 'Object lifetime management rules' section for more of it
int property followers hidden
    int function get()
        return _followers
    endFunction
    function set(int value)
        -- retainAndRelease releases previous _followers object
        -- and owns (retains) a new
        _followers = JValue.releaseAndRetain(_followers, value)
    endFunction
endProperty

int _followers = 0

-- initial setup function where you usually do the things once mod gets installed
function modSetupMethod()
    -- create and retain JFormMap container
    self.followers = JFormMap.object()
endfunction

-- method that gets called once user uninstalls it via MCM
function modUninstallMethod()
    -- release followers container to not pollute game save
    self.followers = 0
endfunction
```

  [1]: https://lh4.googleusercontent.com/-1Q7K-3vT6E8/U1KkKXVAeOI/AAAAAAAAACE/Oief-49GKYs/s0/jcontainers%252520-%252520readme%252520-%252520temp.png "jcontainers - readme - temp.png"
