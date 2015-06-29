import os
import shutil


def makepath(*args):
    path = os.path.join(*args)

    if not os.path.exists(path):
        os.makedirs(path)

    return path


def quotes(s):
    return '"' + s + '"'


def cleanCopyTree(src, dst, **kwargs):
    dirName = os.path.basename(src)
    dest = os.path.join(dst, dirName)
    shutil.rmtree(dest, ignore_errors = True)
    shutil.copytree(src, dest, **kwargs)


ROOT = '..\\'
DIST_DIR = makepath(ROOT + 'dist\\')


class JCLib(object):
    def __init__(self, location):
        import ctypes
        from ctypes import cdll

        print 'loading', JCLib.dllName(), 'at:', location

        self.location = location
        self.lib = cdll.LoadLibrary(location);
        self.lib.JC_versionString.restype = ctypes.c_char_p
        self.lib.JC_produceCode.argtypes = [ctypes.c_char_p]

    @staticmethod
    def dllName():
        return 'JContainers.dll'

    def produceCode(self, into):
        print 'generating papyrus scripts into:', into
        makepath(into)
        self.lib.JC_produceCode(into)

    def versionString(self):
        return self.lib.JC_versionString()

    def runTests(self):
        self.lib.JC_runTests()



class Config(object):
    def __init__(self, mode):
        self.mode = mode
        self._jcLib = None

    @property
    def origin(self):
        return os.path.join(ROOT, self.mode)

    @property
    def pscDir(self):
        return os.path.join(self.origin, 'Data\scripts\source')

    @property
    def compiledDir(self):
        return os.path.join(self.origin, 'Data\scripts')

    @property
    def dataDir(self):
        return os.path.join(self.origin, 'Data')

    @property
    def pluginDir(self):
        return os.path.join(self.origin, 'Data\SKSE\Plugins')

    @property
    def jcLib(self):
        if not self._jcLib:
            self._jcLib = JCLib(os.path.join(self.pluginDir, JCLib.dllName()))

        return self._jcLib


def makeScripts(lib, sourceDir, compiledDir):
    lib.produceCode(sourceDir)

    args = [
        'PapyrusCompiler\PapyrusCompiler.exe',
        quotes(sourceDir),
        '-all',
        '-quiet',
        '-f=' + quotes('PapyrusCompiler\TESV_Papyrus_Flags.flg'),
        '-i=' + ';'.join(map(quotes, [sourceDir, 'PapyrusScripts'])),
        '-o=' + quotes(compiledDir)
    ]

    print 'compiling scripts:', ' '.join(args)

    #os.system(' '.join(args))
    import subprocess
    subprocess.check_call(args)


# def moveTree(root_src_dir, root_dst_dir):
# 	for src_dir, dirs, files in os.walk(root_src_dir):
# 	    dst_dir = src_dir.replace(root_src_dir, root_dst_dir)
# 	    if not os.path.exists(dst_dir):
# 	        os.makedirs(dst_dir)
# 	    for file_ in files:
# 	        src_file = os.path.join(src_dir, file_)
# 	        dst_file = os.path.join(dst_dir, file_)
# 	        if os.path.exists(dst_file):
# 	            os.remove(dst_file)
# 	        shutil.move(src_file, dst_dir)


def copyTree(root_src_dir, root_dst_dir):
    os.system('XCOPY "{}" "{}" /E /I /F /Y /Q'.format(root_src_dir, root_dst_dir))


def setupSkyrimTree(config):
    copyTree('../JContainers/Data', config.dataDir)


def recreatePath(*args):
    path = os.path.join(*args)
    print 'recreating path:', path

    shutil.rmtree(path, ignore_errors = True)

    import time
    while os.path.exists(path):
        time.sleep(0.1)

    os.makedirs(path)
    return path
    

def doLocalInstall(mode, targetDir):
    print 'mode:', mode
    print 'install path:', os.path.abspath(targetDir)

    config = Config(mode)
    setupSkyrimTree(config)
    makeScripts(config.jcLib, config.pscDir, config.compiledDir)

    # real installation
    trueInstallDir = os.path.join(targetDir, 'Data')
    copyTree(config.dataDir, trueInstallDir)

    
def bundlePlugin(mode):
    
    config = Config(mode)
    setupSkyrimTree(config)
    makeScripts(config.jcLib, config.pscDir, config.compiledDir)

    installDir = recreatePath(DIST_DIR, mode)
    shutil.copytree(config.dataDir, os.path.join(installDir, 'Data'), ignore = shutil.ignore_patterns('*.exp', '*.lib', '*.pdb', 'test_data'))
    shutil.make_archive(os.path.join(DIST_DIR, 'JContainers.' + config.jcLib.versionString()), 'zip', installDir)
    shutil.rmtree(installDir, ignore_errors = True)


def bundleDevResources(mode):


    # API usage example,  C API
    # json validator
    devr = recreatePath(DIST_DIR, 'dev_resources')

    #api_usage_example
    cleanCopyTree(os.path.join(ROOT, 'api_usage_example'), devr, ignore = lambda x,y: ['Release', 'Debug'])
    shutil.copy2(os.path.join(ROOT, 'JContainers/src/jc_interface.h'), os.path.join(devr, 'api_usage_example'))

    shutil.copy2(os.path.join(ROOT, 'Release', 'json_validator.exe'), devr)

    shutil.make_archive(devr, 'zip', devr)
    shutil.rmtree(devr, ignore_errors = True)

