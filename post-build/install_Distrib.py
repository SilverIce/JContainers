from jc_install import *

if __name__ == '__main__':
	mode = 'Release'
	bundlePlugin(mode)
	bundleDevResources(mode)

	print 'post-install finished'
