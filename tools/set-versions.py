#!/usr/bin/env python3
import sys
from os import path
import chardet  #pip install chardet

ROOT = ""
VAPI = ""
VMAJ = ""
VPAT = ""

def replace_versions (filepath, replacements):
    """ Replace in-place a file with the given before:after map of strings """
    with open (filepath, 'rb') as infile:
        raw = infile.read (32) # at most
        encoding = chardet.detect(raw)['encoding']
    with open (filepath, 'r', encoding=encoding) as infile:
        lines = infile.readlines ()
    assert (len (lines) > 0)
    for i in range (len (lines)):
        for src, target in replacements.items():
            if lines[i].find (src) is 0:
                lines[i] = src + target + '\n'
    with open (filepath, 'w', encoding=encoding) as outfile:
        outfile.writelines (lines)

def update_header ():
    dst = path.join (ROOT, "src", "JContainers", "src", "jcontainers_constants.h")
    txt = { "#   define JC_API_VERSION           " : VAPI,
            "#   define JC_FEATURE_VERSION       " : VMAJ,
            "#   define JC_PATCH_VERSION         " : VPAT }
    replace_versions (dst, txt)

def update_appveyor ():
    dst = path.join (ROOT, "appveyor.yml")
    txt = { "version: " : "'{}.{}.{}.{{build}}'".format (VAPI, VMAJ, VPAT) }
    replace_versions (dst, txt)

def update_jcrc ():
    dst = path.join (ROOT, "src", "JContainers", "JContainers.rc")
    txt = { ' FILEVERSION ' : '{},{},{},0'.format (VAPI, VMAJ, VPAT),
            ' PRODUCTVERSION ' : '{},{},{},0'.format (VAPI, VMAJ, VPAT),
            '            VALUE "FileVersion", ' : '"{}.{}.{}.0"'.format (VAPI, VMAJ, VPAT),
            '            VALUE "ProductVersion", ' : '"{}.{}.{}.0"'.format (VAPI, VMAJ, VPAT) }
    replace_versions (dst, txt)

if __name__ == '__main__':
   if len (sys.argv) != 4:
        print ("Usage: install.py <API> <Minor> <Patch>")
        exit (1)
   ROOT = path.abspath (path.join (path.dirname (sys.argv[0]), ".."))
   VAPI = sys.argv[1]
   VMAJ = sys.argv[2]
   VPAT = sys.argv[3]
   update_header ()
   update_appveyor ()
   update_jcrc ()
