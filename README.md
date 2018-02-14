# JContainers

A project to extend [Skyrim's Papyrus
scripting](https://www.creationkit.com/index.php?title=Category:Papyrus) with
[JSON formatted](https://json.org/) data structures (arrays and maps).

### Why?

If you are programmer, sooner or later you'll notice the lack of many useful features in Papyrus.
There is no way to:

- Append or erase values from arrays
- Put an array into an array (i.e. no nested arrays)
- Put multiple value types into a single array
- Have associative containers 
- Be able to load or save a data into a file

### Solution

Since there is no source code of the Papyrus virtual machine, it is tricky and no easy to extend the
existing Papyrus Array type or add new data structure type. Thus JContainers implements it's own
data structures, garbage collector and other infernal stuff from scratch. Features offered:

- Data structures: arrays and associative containers (a.k.a. maps or dictionaries)
- Import and export data to and from JSON files
- Embedded, lightweight scripting - [Lua](https://www.lua.org/)
- Interaction with JContainers via C++ interface.

### In depth documentation

[Latest documentation](https://github.com/ryobg/jcontainers/wiki)

### Can I help?

Sure! Feel free to do whatever you think is good - post feature requests, report bugs, improve Wiki
or source code.

# Building from source

### Prerequisites

* [Microsoft Visual Studio 2017](https://www.visualstudio.com/downloads/) 
  Community Edition would suffice, its free. You will need its Visual C++ component at least.
  All the project files are converted to that version, it with a bit of manual work it may work with
  older versions too (e.g. 2013) but issues may arrise. Later versions are more compatible.
* [Boost C++ library](http://www.boost.org/) version 1.66
  The version number is loose requirement, as Boost is largely compatible between the different
  versions, so you may try with different one. An archive of 1.66 can be downloaded
  [here](https://dl.bintray.com/boostorg/release/1.66.0/source/) or from any other place.
* A [Python](https://www.python.org/downloads/) environment for Windows, version 3.4 or later.
  This is needed to run some helper scripts for testing, building distributions and any other small
  helpful tasks. Other versions of Python like 2.x may work or may not.
* The GIT revisioning system and/or GitHub account may help if you want to contribute or work more
  easily with the public repository of this project.

### First time setup

1. Unpack or place somehow the Boost root folder (the one containing `bootstrap.bat`) into the
   JContainer's `dep/boost` folder i.e. you should have the following path available:
   `dep/boost/bootstrap.bat`.
2. Run from the JContainer's root folder the `build_boost.bat` file. It will bootstrap and build the
   neccessary libraries from Boost.
3. Open the `JContainers.sln` file with Visual Studio and Rebuild the whole solution. It will take
   some time.

That's it!

