# Custom Main

# Introduction

This document introduces how to create a custom main when `SimoSim` may not
be the best fit for simulation. [CustomMain.cc](./CustomMain.cc) is a minimal example.

## CMake

CMake targets are ready to use. Instead of using `Simo::Simo_includes_only`,
use `Simo::Simo`. This target includes the headers and the `Simo` library.

A very minimal `CMakeLists.txt` is the following:

```cmake
project(CustomMain VERSION 0.0.1)
find_package(Simo REQUIRED)
add_executable(CustomMain CustomMain.cc)
# Use Simo::Simo to only use libraries and include headers
target_link_libraries(CustomMain PRIVATE Simo::Simo)
```

## Custom main

### `Simo::Context` is all you need
The only thing that it is required is instantiating a `Simo::Context`.

Modules are not strictly required to model a simulation. It is also possible
plainly use `Simo::Context::schedule_*` methods to schedule any type of function:

```cpp
#include <Simo/Simo.h>

void event_function() { std::cout << "Function event" << std::endl; }

int main() {
  Simo::Context sim_ctx;
  sim_ctx.schedule_in(Simo::Time::one, event_function);
  sim_ctx.schedule_in(Simo::Time(10),
                      []() { std::cout << "Lambda event" << std::endl; });
  sim_ctx.run_at(Time(100));
}
```

But remember that modules are a convenient way to model components,
and they are equipped with convenient methods to create full-fledged systems.

### Usage of concrete type of subclasses of `Simo::Module`

One limitation of `SimoSim` is that it manages modules and parameters losing the 
information of the concrete type of modules and parameters. This is a drawback of
the module loading mechanism through collections exposed by shared libraries.

Creating a custom main allows to directly use the concrete types. This is useful
when the system configuration is known at compile time, so the modules that composes
it can be directly instantiated and added to the simulation context with
`Simo::Context::add_module`:

```cpp
#include <Simo/Simo.h>

class TestModule : public Simo::Module {
// Implementation...
};

int main() {
  Simo::Context sim_ctx;
    
  Simo::Parameters params;
  TestModule test_module;
  sim_ctx.add(test_module, params);
  const auto initialize_success = sim_ctx.initialize();
  assert(initialize_success);
  
  // Run simulation
  sim_ctx.run_at(Time(10));
}
```

### Advanced examples

Unit-test at `tests/Simo/MainLoopTests.cc` or any other unit-test that uses `Simo::Context`
are simple examples of how to write custom mains. Also,the source code of `SimoSim`
is a good example at `src/SimoSim/SimoSim.cc`.

## A small digression on `Simo::Context`

Instantiating `Simo::Context` may seem a bit tedious, but it allows to potentially
have multiple simulation contexts in the same executable in parallel.

In SystemC only one simulation context is allowed per process. When the simulation stops,
it cannot be reinitialized. This is a big limitation for unit-testing where multiple simulations 
are required to be executed (hacks with forking the process can work, but it is tedious).
