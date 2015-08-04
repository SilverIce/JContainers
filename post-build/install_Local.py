from jc_install import *
import sys

if __name__ == '__main__':
	
	if len(sys.argv) < 2:
		raise Exception('argc less than 1')

	mode = sys.argv[1]
	doLocalInstall(mode, "\\\\VBOXSVR\\vilya\\PlayOnLinux's virtual drives\\Skyrim\\drive_c\\Program Files\\Mod Organizer\\mods\\JContainers.3.2.4 - WINE")
	print 'post-install finished'
	