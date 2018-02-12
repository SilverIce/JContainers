REM https://sourceforge.net/projects/boost/files/boost/1.66.0/

b2 variant=release,debug link=static threading=multi runtime-link=static --with-serialization --with-date_time --with-filesystem --with-thread --build-dir=garbage --stagedir=boost_lib stage


DEL /q /s garbage

