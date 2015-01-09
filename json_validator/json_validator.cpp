// json_validator.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <jansson/src/jansson.h>
#include <stdio.h>
#include <conio.h>
#include <boost/filesystem.hpp>

int errorCounter = 0;
int filesTotal = 0;

bool is_likely_utf8_bom(FILE *file) {
    if (fseek(file, 0, SEEK_SET) != 0) {
        return false; // error occured, so unk
    }

    const char utf8_BOM[4] = { 0xEF, 0xBB, 0xBF, 0};
    char bytes[4] = { 0, 0, 0, 0 };
    size_t bytes_read = fread_s(bytes, 3, 1, 3, file);

    return bytes_read == 3 && strcmp(bytes, utf8_BOM) == 0;
}

void validate_file(const _TCHAR *path) {

    FILE *file = nullptr;
    errno_t openError = _wfopen_s(&file, path, L"r");
    if (openError || !file) {
        return ;
    }


    json_error_t error;

    auto json = json_loadf(file, JSON_REJECT_DUPLICATES, &error);
    if (!json) {
        wprintf(L"validation failed: %s\n", path);
        printf("line %u column %u\n", error.line, error.column);
        printf("%s\n", error.text);
        printf("source: %s\n", error.source);

        if (is_likely_utf8_bom(file)) {
            printf("likely incorrect encoding. re-save the file using \"UTF-8 without BOM\" encoding format\n");
        }

        printf("\n");

        ++errorCounter;
    }

    json_decref(json);

    fclose(file);

    ++filesTotal;
}

void handle_path(_TCHAR *path) {
    namespace fs = boost::filesystem;

    if (!path || !fs::exists( path )) {
        return;
    }

    if (fs::is_regular_file(path)) {
        validate_file(path);
        return;
    }


    fs::recursive_directory_iterator end_itr;
    fs::path root(path);

    for ( fs::recursive_directory_iterator itr( root ); itr != end_itr; ++itr ) {

        if ( fs::is_regular_file( *itr ) ) {

            validate_file(itr->path().c_str());
        }
    }

}

#define STR(...) #__VA_ARGS__

int _tmain(int argc, _TCHAR* argv[])
{

    if (argc <= 1) {
        return 0;
    }


    int i = 1;

    while (i < argc) {
        _TCHAR *path = argv[i];

        handle_path(path);

        ++i;
    }

    printf("%u errors found. %u files total\n", errorCounter, filesTotal);
    printf("press any key to close\n");

    _getch();
	return 0;
}

