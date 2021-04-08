from setuptools import setup, find_packages
from setuptools.extension import Extension

try:
    from Cython.Distutils import build_ext
    USE_CYTHON = True
except ImportError:
    USE_CYTHON = False

import os

packages = ['lely_io']
package_data = {}
package_dir = {}
for pkg in packages:
    package_data[pkg] = ['*.pxd']
    package_dir[pkg] = os.path.join(*pkg.split('.'))

ext = '.pyx' if USE_CYTHON else '.c'
ext_modules = []
for pkg in packages:
    ext_modules.append(Extension(
        pkg + '.*',
        [os.path.join(*[os.path.dirname(__file__), package_dir[pkg], '*' + ext])],
        language='c', libraries=['lely-io']
    ))

if USE_CYTHON:
    from Cython.Build import cythonize
    ext_modules = cythonize(ext_modules, include_path=['../can'])

setup(
    name='lely_io',
    version='2.4.0',
    description='Python bindings for the Lely I/O library.',
    url='https://gitlab.com/lely_industries/lely-core',
    author='J. S. Seldenthuis',
    author_email='jseldenthuis@lely.com',
    license='Apache-2.0',
    packages=find_packages(),
    package_data=package_data,
    package_dir=package_dir,
    ext_modules=ext_modules
)
