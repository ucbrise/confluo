#!/usr/bin/env python

from setuptools import setup

setup(name='dialog',
      version='0.1.0',
      description='Python Client for DiaLog',
      author='Anurag Khandelwal',
      author_email='anuragk@berkley.edu',
      url='https://www.github.com/ucbrise/dialog',
      package_dir={'dialog': 'dialog'},
      packages=['dialog'],
      setup_requires=['pytest-runner', ],
      tests_require=['pytest-cov', 'pytest', 'thrift>=0.10.0', ],
      install_requires=['thrift>=0.10.0', ],
      )
