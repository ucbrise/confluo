# Installation

Before you can install Confluo, make sure you have the following prerequisites:

* MacOS X or Unix-based OS; Windows is not yet supported.
* C++ compiler that supports C++11 standard
* CMake 2.8 or later
* Boost 1.58 or later

For python client, you will additionally require:

* Python 2.7 or later
* Python Packages: six 1.7.2 or later

For java client, you will additionally require:

* Java 1.7 or later
* ant 1.6.2 or later

## Download

You can obtain the latest version of Confluo by cloning the GitHub repository:

```bash
git clone https://github.com/ucbrise/confluo.git
```

## Configure

To configure the build, Confluo uses CMake as its build system. Confluo only 
supports out of source builds; the simplest way to configure the build would be 
as follows:

```bash
cd confluo
mkdir -p build
cd build
cmake ..
```

It is possible to configure the build specifying certain options based on 
requirements; the supported options are:

* `BUILD_TESTS`: Builds all tests (ON by default)
* `BUILD_RPC`: Builds the rpc daemon and client libraries (ON by default)
* `BUILD_EXAMPLES`: Builds Confluo examples (ON by default)
* `BUILD_DOC`: Builds Confluo documentation (OFF by default)
* `WITH_PY_CLIENT`: Builds Confluo python rpc client (ON by default)
* `WITH_JAVA_CLIENT`: Builds Confluo java rpc client (ON by default)

In order to explicitly enable or disable any of these options, set the value of
the corresponding variable to `ON` or `OFF` as follows:

```bash
cmake -DBUILD_TESTS=OFF
```

Finally, you can configure the install location for Confluo by modifying the
`CMAKE_INSTALL_PREFIX` variable (which is set to /usr/local by default):

```bash
cmake -DCMAKE_INSTALL_PREFIX=/path/to/installation
```

## Install

Once the build is configured, you can proceed to compile, test and install 
Confluo. 

To build, use:

```bash
make
```

or 

```bash
make -j{NUM_CORES}
```

to speed up the build on multi-core systems.

To run the various unit tests, run:

```bash
make test
```

and finally, to install, use:

```bash
make install
```