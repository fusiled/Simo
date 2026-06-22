# Ports and SimoSimo

# Introduction

This document explains basic principle of ports of `Simo` and how to use them
in `SimoSim`.


# Connecting Modules with ports

Ports allow to create connections between modules. The class `Simo::Port` is a
templated class and it is the base class. Any protocol can be built on top of it.
To create a new port class, it is required to implement is the `bool connect(Port* other)`
method in the subclass. This method is called when two ports are connected together
and this is what `SimoSim` uses to connect the ports.

Some templated implementations are already provided in the library to ease integration:
- `Simo::InPort<T>`: Allows to receive a payload of type `T`. It connects to a `Simo::OutPort<T>`
- `Simo::OutPort<T>`: Allows to send a payload of type `T`. It connects to a `Simo::InPort<T>`
- `Simo::BidirectionalPortTyped<OutPayload, InPayload>`: Allows to send a payload of type `OutPayload` and receive a payload of type `InPayload`.
It connects to another `Simo::BidirectionalPortTyped<InPayload, OutPayload>` (notice that
`OutPayload` and `InPayload` are swapped).
- `Simo::BidirectionalPort<T>`: It is an alias for `Simo::BidirectionalPortTyped<T, T>`.
It is a good general-purpose port in case bidirectional communication is needed of
the same payload type.

Look at `Simo/port/Port.h` for the implementation of these classes. Here below a 
recap of the methods that `Simo::BidirectionalPort<T>` provides:
```text
+------------------------------------+
|              [Port A]              |
|  - send_out / clear_out works on X |
|  - receive_in / clear_in works X   |
+------------------------------------+
        |                    ^
        | X goes             | Y goes
        | from A             | from B
        | to B               | to A
        v                    |
+------------------------------------+
|              [Port B]              |
|  - receive_in / clear_in works X   |
|  - send_out / clear_out works on Y |
+------------------------------------+
```

No specific protocol is enforced and both sides of the port can remove the payload
anytime. This is a design choice to allow more flexibility.

# Connecting ports in SimoSim

The configuration file of `SimoSim` allows to connect using the `connections` section.
Look at [system_config.yaml](./system_config.yaml) for an example:

```yaml
connections:
  - ping_module/port: pong_module/port
```

Here `ping_module` and `pong_module` both expose a port named `port`, and these
two are connected together. Look at `tests/collection/PingPongCollection.cc` for 
the implementation of these modules and how the ports are defined. Inside a module,
a port can be created with the `Simo::Module::create_port<{Port-Class}>({port_name})`
method.

To run this example, you need the `PingPongCollection`. This is built by default
when Simo is built. To run the example, use the following command:

```bash
SimoSim --config <EXAMPLE_PATH>/system_config.yaml --search-path <SIMO_BUILD_PATH>
```