# GROSS
> _GRaph cOmpiler Solely for cS241_

GROSS is a research compiler used for studying non-traditional Graph IR, which has advantages on both code optimization and code generations.

This compiler was originally created as a class project in Advanced Compiler Construction(CS241) in UC Irvine. The class was instructed by [Prof. Michael Franz](http://www.michaelfranz.com/).

## Build
This project requires `CMake` (>= 3.5) and any compiler that supports C++11.

First, we need to boot up submodules
```
git submodule init
git submodule update
```
Then configure with CMake
```
mkdir build
cd build
cmake -G <generator> ..
```
There are two important CMake variables:
1. `CMAKE_BUILD_TYPE`. Set to `Release`, `Debug` or [more advance configuration](https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_TYPE.html). Note that if it's set to `Debug` and you're using clang, the following sanitizer will be turned on by default:
   1. Address Sanitizer (ASAN)
   2. Undefined Behavior Sanitizer (UBSAN)
2. `GROSS_ENABLE_UNIT_TESTS` will add unit tests and integration tests to build targets.

For example, the following will generate a debug build with all tests included:
```
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Debug -DGROSS_ENABLE_UNIT_TESTS=True ..
```

Finally, build it
```
ninja
```
Use `check-units` to only run the unit tests, `check-integrations` for integration tests, and `check-gross` for all the above.
```
ninja check-units
ninja check-integrations
ninja check-gross
```

## The `gross` Driver
The `gross` tool under `src/Driver` simply runs the entire compilation pipeline and provides options for dumping intermediate IR graph. Please run `gross --help` for available options.

## Language / Target Architecture Specs
 - [Programming Language(PL241)](doc/lang-spec.pdf)
 - [Target Architecture](doc/DLX.pdf)

## Design
The Graph IR is inspired by TurboFan JIT compiler in [V8 javascript engine](https://v8.dev),
which is also called _"Sea of Nodes"_ IR.
 - [C Click, KD Cooper."Combining analyses, combining optimizations"](https://dl.acm.org/citation.cfm?id=201061)
   - The original "Sea of Nodes" paper. (Jump to Chap 6 and 7 for TL;DR)
 - [TurboFan TechTalk presentation](https://docs.google.com/presentation/d/1sOEF4MlF7LeO7uq-uThJSulJlTh--wgLeaVibsbb3tc/edit#slide=id.g54ccc405e_2102)
 - [TurboFan IR](https://docs.google.com/presentation/d/1Z9iIHojKDrXvZ27gRX51UxHD-bKf1QcPzSijntpMJBM/edit#slide=id.g19134d40cb_0_193)
