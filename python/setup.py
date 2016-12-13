from distutils.core import setup
from distutils.extension import Extension

try:
    from Cython.Distutils import build_ext
    USE_CYTHON = True
except ImportError:
    USE_CYTHON = False

import os

pkg = 'lely_io'
pkg_dir = os.path.join(os.path.dirname(__file__), pkg)

ext = '.pyx' if USE_CYTHON else '.c'
extensions = [Extension(pkg + '.*', [os.path.join(pkg_dir, '*' + ext)],
                        language='c', libraries=['lely-io'])]

if USE_CYTHON:
    from Cython.Build import cythonize
    extensions = cythonize(extensions)

setup(
    name=pkg,
    version='1.0.1',
    description='Python bindings for the Lely I/O library.',
    url='https://gitlab.com/lely_industries/io',
    author='J. S. Seldenthuis',
    author_email='jseldenthuis@lely.com',
    license='Apache-2.0',
    packages=[pkg],
    package_data={pkg: ['*.pxd']},
    package_dir={pkg: pkg_dir},
    ext_modules=extensions
)

