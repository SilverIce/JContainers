from jc_install import *
import sys

if __name__ == '__main__':
    if len(sys.argv) < 2:
        raise Exception('argc less than 1')

    location = sys.argv[1]
    test_args = ('', '--gtest_filter=*.*',)
    lib = JCLib(os.path.join(location, JCLib.dllName()))

    try:
        if not lib.runTests(test_args):
            raise Exception("A test didn't pass")
    except BaseException as e:
        print("The tests has failed with unexpected error:", e)
        raise
    except:
        print("The tests has failed with unexpected error:", sys.exc_info()[0])
        raise

    