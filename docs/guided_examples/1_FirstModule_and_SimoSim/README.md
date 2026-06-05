# FirstModule SimoSim 

## Introduction
The documents highlights some location in the source of [FirstModule.cc](./FirstModule.cc)
to better understand how to write your own `Module` class and how to use it
in `SimoSim`, a general purpose executable to run simulations.


### You first module

Modules are the basic block of the simulation. They have the ability to
schedule events in the simulation loop. To create a new module, it is
enough to inherit from `Simo::Module` class. 


#### Module initialization
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

#### Module event

`event_log` runs first at time zero, and it re-schedules itself every `period` time units
using the simulation context. Look for `Simo/core/Context.h` for the difference between
`schedule_at` and `schedule_in` of .
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
