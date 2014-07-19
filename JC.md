### Motivation & Overview

Papyrus lacks of convenient data structures such as dynamic arrays or associative containers. Script language should be simple but not underdeveloped.

This plugin attempts to add missing functionality. Although it is not native functionality and it will never have the nice brackets to access items that the default Papyrus Array has, I believe it is still better than nothing.

Current version implements array and associative (map or dictionary) containers: JArray (array container), JMap and JFormMap (both associative containers) and few convenient wrappers: JDB and JFormDB (databases).

### Reference

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
string string = JArray.getStr(array, 0)
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
; // equivalent ways to do same things.
; // all count functions return zero as new containers are empty
JValue.count(array) == JArray.count(array) == JValue.count(map)
; // write container content into file:
JValue.writeToFile(map, "map.txt")
JValue.writeToFile(array, "array.txt")
```

#### JDB

Take it as a global entry point or database - you put information in it under a string key "yourKey", and then you can access to it from any script in the game. There is only one JDB in game, so each time you access it you access that one, single JDB. It is an **associative** container like JMap (it is, in fact, JMap internally), but the script interface is slightly different.

Typical JDB usage would involve:

1. Setup (during mod installation) where you specify `root key` name. In example below root key is `frostfall`. **Choose root name carefully to avoid clashes with rest of JDB root keys and with JFormDB storage names**.
2. Access data

```
;// 1. Setup procedure
int frosfallData = JValue.readFromFile("frostfall_config.json")
JDB.setObj("frostfall", frosfallData)

;// 2. read/write data later in another script:
int lighttignMode = JDB.solveInt(".frostfall.campfileLightingMode")
JDB.solveIntSetter(".frostfall.campfileLightingMode", 1)
```



#### JFormDB

Provides a convenient way to associate values with a form. You may find it looking like a mix of JMap and JDB - like JDB, there is only one JFormDB in game, and it's associative container, like JMap. Also it supports path resolving.
To store or retrieve value form-key and string path must be passed:

```lua
; // store...
form me
JFormDB.setFlt(me, ".yourModFormStorage.valueKey", 10)
JFormDB.setStr(me, ".yourModFormStorage.anotherValueKey", "name")
; // and retrieve values
float value = JFormDB.getFlt(me, ".yourModFormStorage.valueKey")
```

String path must always consist of two parts: `formStorageName` and `valueKey`

`valueKey` is a key used to retrieve value or create {valueKey, value} association for a form.

`formStorageName` is a JFormMap containing {formKey, {valueKey, value}} associations.
It was added to avoid possible collisions: one mod may occasionally override value written by another mod if I'd allowed simple paths without storage name part. The fact that it is a separate storage makes it possible to access that storage, delete it without any risk to delete another mod data:

```lua
; // Will destroy everything "yourModFormStorage" contains
JDB.setObj("formStorageName", 0)
```

How any `set*` function works internally:
Once a value gets assigned via `JFormDB.set*(formKey, ".formStorageName.valueKey", value)`, JFormDB looks for `formStorageName` in the JDB (or creates it if it isn't found) and then looks for the JMap entry associated with the form key (or it creates an entry if none is found) and then creates a {valueKey, value} pair.

Slightly more advanced usage:

```lua
; // Will destroy everything associated with 'me' form in "yourModFormStorage" storage
JFormDB.setEntry("yourModFormStorage", me, 0)
 
; // Custom entry type
JFormDB.setEntry("yourModFormStorage", me, JArray.object())
```

### API usage notes

First and foremost - game will not crash no matter what data you will pass into JContainer functions. The following happens if function gets called with invalid input (state when input cannot be handled properly):

All functions returning new container return zero identifier. For. ex `JValue.readFromFile("")` returns 0 because of invalid file path. Zero identifier means non-existing object. It’s ok to pass it into other functions - in that case function will return default value.

All functions that read container contents (such as `getFlt`, `solveFlt`, `getStr`, `count`, `allKeys` and etc) return default value. In case function returns integer or float - default value is 0, string or form - `None`, container identifier - 0.

### Object persistence

Every container object persists in save file until it (container) gets destroyed. When save performed all objects are saved and all objects are resurrected when save file gets loaded.

### JSON serialization

As said above, it’s possible to serialize/deserialize container data (write to or read from an external file). While numbers and strings serialized in natural way, store form information is slight tricky as JSON knows nothing about Skyrim forms (and also because global form id depends on mod load order). Serialized form is a string prefixed with `"__formData"`, plugin file name and local or global form id (hex or decimal number).

Example of serialized JMap containing player's form associated with `"test"` key:
```json
{
    "test": "__formData|Skyrim.esm|0x14",
    "name": "Elsa",
    "level": 2
}
```
Serialized array and nested form-map container:
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
Serialization:
```lua
int playerData = JMap.object()
JMap.setForm(playerData, "actor", playerForm)
JMap.setInt(playerData, "level", playerForm.GetLevel())
JValue.writeToFile(playerData, "Data/playerInfo.txt")
```
Deserialization:
```lua
int data = JValue.readFromFile("Data/playerInfo.txt")
int level = JValue.solveInt(data, ".level")
form player = JValue.solveForm(data, ".actor")
```

### Path resolving

This feature simplifies an access to values of nested objects via group of `solve*` and `solve*Setter` functions. Each function takes path specifier, which determines in which key to search for a value. For example:

```
solveInt(objectA, ".keyA[4].keyB")
```
retrieves a value which is associated with keyB of JMap, which located at 4-th index of JArray, which is associated with keyA of objectA-JMap. Huh.

`solve*Setter` changes (assigns) a value. Also there is an optional `createMissingKeys` argument - if enabled, will insert any missing JMap key during path traversal. For example, calling `solveFltSetter(objectA, ".key1.key2", 3.14)` on an empty objectA will create new JMap B containing `{"key2", 3.14}` pair and associate objectA with new JMap (i.e. `{"key1", B}` pair will be created).

More examples:
```json
{
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

Feature allows execute functions on collection (container) elements. It’s accessible via solve* functions.
Syntax:

* @function
* @function.path.to.element
* path.to.container@function
* path.to.container@function.path.to.element

path.to.container - is the path to retrieve collection.

function - is the function that will be applied on each collection element. Currently only few functions implemented:

* minNum, maxNum (search for min or max number, works with any number type (int or float))
* minFlt, maxFlt - the same as above, accepts float values only
* minInt, maxInt - the same as above, accepts integer values only

path.to.element - is the path to retrieve element.

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

In order to make path resolving and collection operators features function properly sting-keys should not contain point, square brackets or `@` characters. For instance, the following code will fail to work:
```lua
obj = { "invalid.key" : {"k": 10} }

solveInt(map, ".invalid.key.k") is 0

// although it's still possible to access value in another way:
getObj(map, "invalid.key") is {"k": 10}
```
This convention applies to every key-string, not just JMap key - it affects JFormDB storage name and keys, JDB.setObj key. Key naming shouldn't matter if it not involved in path resolving.

### Number conversion notes

Functions accessing number (`getFlt`, `solveFlt`, `getInt`, `solveInt`) may convert it e.g. able to read any kind of number no matter whether stored number is integer or real value. While the rest of `get*` and `solve*` functions may fail to perform conversion and return default value.


### Object lifetime management rules

Each time script creates new string or papyrus array, Skyrim allocates memory and automatically frees it when you do not need that string or array or something else.

Internally all containers are C++ objects, Skyrim knows nothing about them and unable to manage their lifetime and memory.

The lifetime management model is based on object ownership. Any container object may have one or more owners. As long as an object has at least one owner, it continues to exist. If an object has no owners it gets destroyed.

The rules:

- to prevent destruction you must own container (use `JValue.retain` function)
- when you do not need that object you must release it (use `JValue.release`)

**The caller of `JValue.retain` is responsible for releasing object. Not released object will remain in savefile forever.**

The lifetime model implemented using simple owner(reference) counter. Each object have a such counter. Each time object gets inserted into another container or `JValue.retain` used reference counter increases. Each time object gets removed from container or released via `JValue.release` reference counter decreases.
If reference counter reaches zero, object temporarily owned for roughly 10 seconds, during this perod of time it have a  'last chance to survive' - and gets destroyed if nobody owned it.

Newly created object (created with `object`, `objectWith*`, `all/Keys/Values` or `readFromFile` function) also have that 'last chance'.

Illustration shows the idea:

![test][1]

## Tutorial

### Simple example

Suppose you want to store some actor related information (let it be player’s followers and their mood):

```lua
function storeFolloverMood(form follower, float mood)
    ;// function creates "followers" storage and then creates
    ;// (follower, entry) associations
    JFormDB.setFlt(follower, ".followers.mood", mood)
endfunction

float function followerMood(form follower)
    ;// fetch follower mood
    return JFormDB.getFlt(follower, ".followers.mood")
endfunction

; method that gets called once user uninstalls your mod
function modUninstallMethod()
    ;//  destroy association to not pollute game save and precious RAM
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
;// let it be .classicPreset or .winterHorkerPreset string
string currentPreset

;// use function each time you need re-read preset from a file
function parseConfig()
    ;// that’s all. presets are already in Skyrim
    ;// readFromFile returns root map container
    ;// it may return zero if file not exist or it can not be parsed (not JSON format or you have accidentally added extra coma)
    int config = JValue.readFromFile("Data/preset.txt")
    ;// put config into DB - associate key and config
    JDB.setObj("frostfall", config)
    currentPreset = ".classicPreset"
endfunction

bool function axeDurabilityEnabled()
    ;// solveInt like any solve* function tries to find (solve) value for given path
    ;// current path is ".frostfall.classicPreset.axeDurability"
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
    ;//  read config file from game root folder and associate it with "scaleMod" key
    JDB.setObj("scaleMod", JValue.readFromFile("scale.txt"))
Endevent

function setScale(float scale)
    objectreference plr = GetTargetActor()
    
    ;// retrieve config
    int config = JDB.solveObj(".scaleMod")
    
    ;// iterate over array & calculate bone scale
    int i = JArray.count(config)
    while(i > 0)
        i -= 1
        ;// fetch sub-array. it can be ["NPC Head [Head]", 0, -0.33] for instance
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
    ;// get follower entry to write into it
    int entry = getActorEntry(follower)
    ;// write mood into follower entry
    JValue.solveFltSetter(entry, ".mood", mood)
endfunction

function addFollowerVictim(form follower, form victim)
    ;// get follower entry to write into it AND then get victims array
    int victims = JValue.solveObj(getActorEntry(follower), ".victims")
    ;// add victim into array
    JArray.addForm(victims, victim)
endfunction

float function followerMood(form follower)
    ;// get follower entry AND fetch mood
    return JValue.solveFlt(getActorEntry(follower), ".mood")
endfunction

float function followerAnger(form follower)
    return JValue.solveFlt(getActorEntry(follower), ".anger")
endfunction

;// find (or create new if not found) per-actor information containing mood, anger and array of victims
int function getActorEntry(form actor)
    int entry = JFormMap.getObj(self.followers, follower)
    ;// if no entry found - create new from prototype-string
    if !entry
        entry = JValue.objectWithPrototype("{ \"mood\": 0, \"anger\": 0, \"victims\": [] }")
        JFormMap.setObj(self.followers, follower, entry)
    endif

    return entry
endfunction

;// property hides all black magick - retains & releases object
;// see 'Object lifetime management rules' section for more of it
int property followers hidden
    int function get()
        return _followers
    endFunction
    function set(int value)
        ;// retainAndRelease releases previous _followers object
        ;// and owns (retains) a new
        _followers = JValue.releaseAndRetain(_followers, value)
    endFunction
endProperty

int _followers = 0

;// initial setup function where you usually do the things once mod gets installed
function modSetupMethod()
    ;// create and retain JFormMap container
    self.followers = JFormMap.object()
endfunction

;// method that gets called once user uninstalls it via MCM
function modUninstallMethod()
    ;// release followers container to not pollute game save
    self.followers = 0
endfunction
```

  [1]: https://lh4.googleusercontent.com/-1Q7K-3vT6E8/U1KkKXVAeOI/AAAAAAAAACE/Oief-49GKYs/s0/jcontainers%252520-%252520readme%252520-%252520temp.png "jcontainers - readme - temp.png"
