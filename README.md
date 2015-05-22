###JContainers

The main goal of the project is to extend Papyrus with JSON-based data structures (arrays and maps).

###Why bother?

If you are programmer, sooner or later you'll notice the lack of many, many must-have features in Papyrus. First of all, the only data structure in Papyrus is Array. There is no way to:

- append, erase values from arrays
- put an array into an array (e.g. no nested arrays)
- put multiple value types into a single array

And I didn't mentioned the lack of associative containers or impossibility to load or save a data into a file. Though, Papyrus is a specialized language, we shouldn't expect much.

###The solution

There is no easy way to extend existing Array (which would require to change Papyrus VM, which is tricky since we have no source code) or add new data structures into VM, thus JContainers implements it's own data structures, garbage collector and other infernal stuff from scratch. Features, offered by JContainers:

- Data structures (arrays, dictionaries)
- Import and export data to and from JSON files
- Embedded Lua - it possible to use powerful Lua
- C++ API - interaction with JContainers via C++ interface. See "developer resources" archive for usage example.

###Links

[Latest documentation](https://github.com/SilverIce/JContainers/wiki)

###Can I help?

Sure! Feel free to do whatever you think is good - post feature requests, report bugs, improve Wiki or source code.
