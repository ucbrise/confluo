import glob
import os
import sys

# append module root directory to sys.path
SCRIPT_DIR = os.path.abspath(__file__)
CLIENT_ROOT_DIR = os.path.dirname(os.path.dirname(SCRIPT_DIR))
sys.path.append(os.path.join(CLIENT_ROOT_DIR, 'dialog'))

# append thrift libraries to sys.path
PROJECT_ROOT_DIR = os.path.dirname(CLIENT_ROOT_DIR)
for libpath in glob.glob(os.path.join(PROJECT_ROOT_DIR, 'external', 'thrift*', 'lib', 'py', 'build', 'lib.*')):
    if libpath.endswith('-%d.%d' % (sys.version_info[0], sys.version_info[1])):
        sys.path.append(libpath)
        break

print sys.path
