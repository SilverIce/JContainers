import os
import sys

class JCLib(object):
    def __init__(self, location):
        import ctypes
        from ctypes import cdll
        print 'loading JCLib at:', location
        self.lib = cdll.LoadLibrary(location);

    def runTests(self):
        self.lib.JC_runTests()


if __name__ == '__main__':

    # print sys.argv
    # location = sys.argv[1]
    # lib = JCLib(location)
    # lib.runTests()

    try:
        print sys.argv
        location = sys.argv[1]
        lib = JCLib(location)
        lib.runTests()
    except BaseException as e:
        print 'Error:', e
        #raise
    except:
        print "Unexpected error:", sys.exc_info()[0]
        #raise

    input("Press Enter to continue...")

