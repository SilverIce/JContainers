@echo off
REM https://sourceforge.net/projects/boost/files/boost/1.66.0/
REM https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.7z

pushd "%~dp0\..\dep\boost\"
if %errorlevel% neq 0 (
    echo Unable to locate the Boost folder
    exit /b 1
)

if not exist b2.exe ( 
    echo b2 not found, bootstrapping Boost...
    call bootstrap.bat
    if %errorlevel% neq 0 (
        echo Unable to bootstrap, exiting...
        popd
        exit /b 2
    )
)

b2 variant=release,debug link=static threading=multi runtime-link=static address-model=64 --with-serialization --with-date_time --with-filesystem --with-thread --build-dir=garbage --stagedir=boost_lib stage
if %errorlevel% neq 0 (
    echo Boost build failed, exiting...
    popd
    exit /b 3
)
    
REM RMDIR /Q /S garbage

popd

