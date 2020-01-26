from jc_install import *
import sys

if __name__ == '__main__':
    if len(sys.argv) < 2:
        raise Exception('argc less than 1')

    location = sys.argv[1]
    test_args = ('', '--gtest_filter=*.*',)

    try:
        print(sys.argv)
        lib = JCLib(os.path.join(location, JCLib.dllName()))
        if not lib.runTests(test_args):
            raise Exception("Tests has failed")
    except BaseException as e:
        print('Error:', e)
        raise
    except:
        print("Unexpected error:", sys.exc_info()[0])
        raise

    