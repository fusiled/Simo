# Simo - The pragmatic simulator of computer systems

Simulation is hard. Harnessing the underlying infrastructure must be easy.

## What is this?

Simo is a C++ library that allows to simulate computer systems. It
uses a discrete-event system where a cycle-based approach is encouraged.
This library has been created after years of experience with computer systems
simulators (both in research and in industry) and it tries to create something that
it is missing: a simple, easy to use library that does what you need.

## Build

Boost and CMake >= 3.31.0 are the only real build dependencies. Build Simo with CMake. 
The build will produce `libSimo.<extension>`. `<extension>` depends on the building target platform and compilation flags.

The nix flake simplifies the setting up the dependencies. After having installed nix, just run `nix develop` to enter 
a virtual environment with all the required tools installed. The Github actions are instrumented using the nix flake.

## Features

- Dynamic detection and loading of collections

See Documentation for more details.

## Motivations of creating Simo

Simulators publicly available pretends to be easy to use, but in practice they are not. Creating a new solution from
scratch is not easy, but it can solve the problems of the exiting ones. More information in [docs](./docs/README.md).

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
