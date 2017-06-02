from distutils.core import setup, Extension

nativeETF = Extension(name='nativeETF',
                      sources=['native_src/library.cpp'],
                      include_dirs=['native_src/'])

setup(name='PyrlangETF',
      version='1.0',
      description='C++ Implementation of External Term Format codec in Pyrlang',
      ext_modules=[nativeETF])
