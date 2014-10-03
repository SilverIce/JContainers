from jc_install import *
import sys

if __name__ == '__main__':
	
	if len(sys.argv) < 2:
		raise Exception('argc less than 1')

	mode = sys.argv[1]
	doLocalInstall(mode, "../../")
	print 'post-install finished'
	