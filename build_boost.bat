REM https://sourceforge.net/projects/boost/files/boost/1.66.0/
REM https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.7z

cd dep\boost\

if not exist b2.exe (
    call bootstrap.bat
)

b2 variant=release,debug link=static threading=multi runtime-link=static address-model=64 --with-serialization --with-date_time --with-filesystem --with-thread --build-dir=garbage --stagedir=boost_lib stage

cd ..\..\
