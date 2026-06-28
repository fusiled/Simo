# FirstModule SimoSim 

## Introduction
The document highlights some location in the source of [FirstModule.cc](./FirstModule.cc)
to better understand how to write your own `Module` class and how to use it
in `SimoSim`, a general purpose executable to run simulations.

It is assumed that this example is executed inside the nix development environment
provided by [`../flake.nix`](../flake.nix).

## You first module

Modules are the basic block of the simulation. They have the ability to
schedule events in the simulation loop. To create a new module, it is
enough to inherit from `Simo::Module` class. 


### Module initialization
All the classed are declared in the
include `Simo/Simo.h` and they are contained in the `Simo` namespace.

```cpp
#include <Simo/Simo.h>
// ...
class SIMO_PUBLIC FirstModule : public Module {
public:
    InitializationStatus initialize(Context& sim_ctx_p,
                                  const Parameters& parameters) override {
       // Initialization
    }
    // ...
};
```

The `initialize` method allows to initialize a module with the initial state of
the simulation.

`Context& sim_ctx_p` is a data-structure that represents the 
global state of the simulation. Events are scheduled using this class. It is good
practice to call the method of the inherited class `Module::initialize` to start
the initialization. This will set the simulation context and do checks on the 
parameters.

`const Parameters& parameters` stores a set of parameters that the class can query
and use. In the example, the period is fetched and set:

```cpp
period = parameters.get<Time>("period")->value();
```

In the initialization, a first event is scheduled at time zero to call the `log_event` method:

```cpp
sim_ctx().schedule_at(Time::zero, [this]() { log_event(); });
```

Other interesting content in the `initialize` method:
- a new statistic is created with `create_statistic` method.
- logging is set up to save the log at `tutorial_module.log`

### Module event

`event_log` runs first at time zero, and it re-schedules itself every `period` time units
using the simulation context. Look at `Simo/core/Context.h` for the difference between
`schedule_at` and `schedule_in` .
Source of the method:

```cpp
void log_event() {
    // schedule_in schedule and event at current_time + period
    ++(*num_events);
    log(Log::LogLevel::INFO, [this]() {
      return std::format("{}, Hello from TestModule!", num_events->value());
    });
    sim_ctx().schedule_in(period, [this]() { log_event(); });
}
```

This function above increase the statistic, and it logs the event in the logger.

### Parameters for the module

`Parameters` class defines a set of parameters. Parameters are organized in a
trie-like structure. The easiest way to define parameters for a new module is to
extend the `Parameters` class, and add parameter definitions in the constructor.

```cpp
class FirstModuleParameters : public Parameters {
 public:
  // Parameters describe inputs of a Module.
  // Parameters with a default value are added with `trie.add<>`
  // and without a default with `trie.add_unset<>`
  FirstModuleParameters() {
    // validator method is used to verify if a parameter is valid
    trie.add_unset<Time>("period").validator(
        [](const auto& t) { return t > Time::one; });
    trie.add<std::string>("log_level", "WARNING");
  }
};

```

## Expose modules and parameters for SimoSim

`SimoSim` is an executable able to detect shared libraries at runtime that contains
modules.  A shared library needs to expose a function named `simo_get_collection`and add `SIMO_COLLECTION_DECLARATION`
**only once per library**.
With this function, `SimoSim` can load a collection of factories. A Factory tells
`SimoSim` how to instantiate a module and the associated parameters.


## Build the test module as a shared library

Use the usual CMake commands to build the shared library that contains the `FirstModule`. Note
it is not required to link against the shared library of Simo in this case: only
the header files are sufficient. The build process will create a shared library
named `libFirstModuleCollection.<extension>`.

## Run your first module with `SimoSim`

Assuming:
- The shared library has been built in `<COLLECTION_PATH>`
- The path to the example is `<EXAMPLE_PATH>`
Run `SimoSim` with the following command:

```bash
 SimoSim --config <EXAMPLE_PATH>/system_config.yaml --search-path <COLLECTION_PATH>
```

This command will run the simulation as specified in `system_config.yaml`. Please
look at this file and look at the structure. Note:
- The `collector` module. This one allows to dump statistics
- The `parameters` section allows to specify the parameters for a module instance
- `connections` is empty. Connections are going to be explained in another example.
- `simulation` section tells how long the simulation should run.


## Next Steps

Try to create your own module and pack it into a collection.

A more complete example is the `PingPongCollection` at 
`tests/collection/PingPongCollection.cc`.

The second example will show how to deal with module ports.
