# Confluo Documentation

Confluo uses:

* MkDocs to generate documentation html
* Doxygen to generate API documentation

as a part of its build. 

If the `BUILD_DOCS` variable is set to `ON` while configuring the
build using CMake, the documentation is automatically built with the source.

Confluo documentation is spread across text files formatted using markdown. You
can read these text files directly, with [`index.md`](src/index.md) as your 
starting point.
