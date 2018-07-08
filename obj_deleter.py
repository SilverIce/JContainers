import os
import shutil

buildDir = '.\\build'

for dirName, subdirList, fileList in os.walk(buildDir):
    # print('Found directory: %s' % dirName)
    for fname in fileList:
        filePath = os.path.join(dirName, fname)
        if os.path.is_file(filePath) && filePath.endswith(".obj"):
            os.remove(filePath)
