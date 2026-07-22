# Simo - Simple Modeling

Simulation of computer architectures is hard. Harnessing the underlying infrastructure
must be easy.

## What is this?

Simo is a C++ library that allows to simulate computer systems. It  uses a
discrete-event system where a cycle-based approach is encouraged.
This library has been created after years of experience with computer systems
simulators (both in research and in industry) and it tries to create something that
it is missing: a simple, easy to use library that does what you need.

## Documentation

**[GitHub Pages](https://fusiled.github.io/Simo/)** stores Doxygen documentation and guides (Markdown files inside `docs` folder).

## Get the repository and build in 3 commands
Assuming [nix](https://nixos.org/learn/):
```bash
git clone https://github.com/fusiled/Simo
cd Simo
nix build
```
The build is going to be accessible in the `result` folder.

## Build

Clone Simo and initialize the submodules:

Build dependencies:
- Clang (recommended) or gcc
- CMake >= 3.31.0
- Boost ([Boost.Test](https://www.boost.org/libs/test), [Boost.TypeIndex](https://www.boost.org/libs/type_index))
- [Glaze](https://github.com/stephenberry/glaze)
- [Doxygen](https://www.doxygen.nl/index.html) (for documentation)

The build will produce `libSimo` (core library) and `SimoSim` (a general purpose
executable to run simulations).

The [nix](https://nixos.org/learn/) flake simplifies the setup of the dependencies. With nix installed you can:
- Build the package with `nix build`. The build will be accessible in the `result` folder.
- Run `nix develop` to enter a virtual environment with all the required tools installed.
The GitHub actions run entering the virtual environment of `nix develop`.

## First Steps

Look inside [docs/guided_examples](./docs/guided_examples/README.md).

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

Default compiler is clang. There is support for gcc build as well. With `nix`, use `nix develop .#gcc` to obtain
a development environment that uses gcc as compiler.

### IDEs
You can see [this guide](./support/ides/clion/README.md) on how to use nix
binaries to build in CLion.

## To Do
- Create a collector with periodic window collection functionality
- More documentation and examples
- Benchmark Simo against Gem5 and SystemC
