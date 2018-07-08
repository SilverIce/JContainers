import os
import shutil

rootDir = '..\\boost_1.59\\boost_1_59_0'
destDir = '.\\dep\\boost'
for dirName, subdirList, fileList in os.walk(rootDir):
    # print('Found directory: %s' % dirName)
    dirNameUniv = dirName.replace('..\\boost_1.59\\boost_1_59_0\\', '')
    for fname in fileList:
        #univFilePath = os.path.join(dirNameUniv, fname)
        destPath = os.path.join(destDir, dirNameUniv, fname)
        if os.path.exists(destPath):
        	src = os.path.join(dirName, fname)
        	shutil.copy2(src, destPath)
        	#print(src,'to',destPath)
        else:
        	print('no dest', destPath)


