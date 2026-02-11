Find the tutorial in [tutorial folder](./tutorial)

# Motivations

Simulators publicly available pretends to be easy to use, but in practice they are not.

[gem5](https://www.gem5.org/) is the most popular simulator. Used a lot in research.
The main problem is its complexity. To learn the basics, the most common resource is a book.
This is too much to pick up something quick. And then there is the mixture of C++
and Python. Countless hours are spent learning (and debugging) before obtaining something useful.

[SystemC](https://github.com/accellera-official/systemc) is tries to mimic hardware components with a terrible C++ API.
Well adopted in industry. It is supposed to work in theory, but in practice it is terrible (learning the library is not
trivial, cannot have proper unit-tests infrastructure).

Creating a new solution from scratch is not easy, but it can solve the problems of the exiting ones.