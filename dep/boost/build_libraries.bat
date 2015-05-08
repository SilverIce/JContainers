b2 variant=release link=static threading=multi runtime-link=static --with-date_time --with-serialization --with-filesystem --with-thread --build-dir=garbage --stagedir=boost_lib stage


DEL /q /s garbage

