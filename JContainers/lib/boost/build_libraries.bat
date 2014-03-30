b2 variant=release link=static threading=multi runtime-link=static --with=thread,serialization --build-dir=garbage --stagedir=boost_lib stage

@echo off

DEL /q /s garbage

echo on

