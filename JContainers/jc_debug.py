import os
import sys
import ctypes

class JCLib(object):
    def __init__(self, location):
        from ctypes import cdll
        print('loading JCLib at:', location)
        self.lib = cdll.LoadLibrary(location)

    def runTests(self, args):
        print(len(args))
        stringArray = (ctypes.c_char_p * len(args))()
        for i in range(len(args)):
            stringArray[i] = args[i].encode('utf-8')

        succeed = self.lib.JC_runTests(len(args), ctypes.byref(stringArray))
        print('The tests succeed', succeed)


if __name__ == '__main__':

    # print sys.argv
    # location = sys.argv[1]
    # lib = JCLib(location)
    # lib.runTests()
        
    args = ('', '--gtest_filter=form_entry_ref._',)

    try:
        print(sys.argv)
        location = sys.argv[1]
        lib = JCLib(location)
        lib.runTests(args)
    except BaseException as e:
        print('Error:', e)
        #raise
    except:
        print("Unexpected error:", sys.exc_info()[0])
        #raise

    input("Press Enter to close...")

