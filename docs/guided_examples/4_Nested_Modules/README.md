# Nested Modules

This document briefly explains how to create modules inside other modules.
[NestedModule.cc](./NestedModule.cc) is a minimal example.

## Nested modules

A module can be instantiated inside another using the usual flow of creating a module
and using parameters to initialize it. `Simo::Module::create_child` allows to 
create modules with a lifetime associated to the parent.

`Simo::Module::create_child` is a utility method to create the name of a child.

The usual flow to create a nested component is:

```cpp

class NestedModule : public Simo::Module {
 // implementation
};

class RootModule : public Simo::Module {
public:
    Simo::InitializationStatus initialize(Simo::Context& ctx,
                                        const Simo::Parameters& p) override {
       // usual Module initialization
       nested_module = &create_child<NestedModule>();
       // Propagate params to nested component through a copy
        Simo::Parameters nested_param = p;
        nested_param.name(name_of_child("nested_module"));
        auto nested_module_success = nested_module->initialize(ctx, nested_param);
        return !nested_module_success.success()
                   ? nested_module_success
                   : Simo::InitializationStatus::ok(this);                                 
    }    
    NestedModule* nested_module = nullptr;
};

```
