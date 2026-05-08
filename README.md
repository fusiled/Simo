# Simo - Simple Modeling

Simulation of computer architectures is hard. Harnessing the underlying infrastructure
must be easy.

## What is this?

Simo is a C++ library that allows to simulate computer systems. It  uses a
discrete-event system where a cycle-based approach is encouraged.
This library has been created after years of experience with computer systems
simulators (both in research and in industry) and it tries to create something that
it is missing: a simple, easy to use library that does what you need.

## Build

Build dependencies:
- CMake >= 3.31.0
- Boost ([Boost.Test](https://www.boost.org/libs/test),
[Boost.TypeIndex](https://www.boost.org/libs/type_index))
- [Glaze](https://github.com/stephenberry/glaze)

Library usage dependencies (headers-only):
- Boost ([Boost.TypeIndex](https://www.boost.org/libs/type_index))
- [Glaze](https://github.com/stephenberry/glaze)

The build will produce `libSimo`.

The nix flake simplifies the setting up the dependencies. With nix installed you can:
- Build the package with `nix build`
- Run `nix develop` to enter a virtual environment with all the required tools installed.
The GitHub actions runs entering the virtual environment of `nix develop`.

## Features

- Loading components from shared objects at runtime ([examples](./tests/collection/CollectionTest.cc))

## Motivations of creating Simo

Simulators publicly available pretends to be easy to use, but in practice they are not. 
More information in [docs](./docs/README.md).

## Code Contributions

Contributions are welcome. Contributions **MUST** be:

- Easy to understand
- Easy to maintain

This can be achieved with:

- Simple logic and simple code
- Good documented code
- Proper unit-testing coverage

## To Do
- Create a collector with periodic window collection functionality
- Create logging system for components 
- More documentation and examples 
- Error propagation upon module initialization 
- Error config in Simo::Context 
- Create a generic executable that can have an input yaml to describe components, parameters and connections 
- Benchmark Simo against Gem5 and SystemC
