# Use Nix packages in CLion

Based on [this](https://gist.github.com/pmenke-de/2fed80213c48c2fe80891678f4fa3b42),
but reworked to use flakes.

1. Run `support/ides/clion/setup-symlinks.sh` to expose some nix binaries
2. In `Settings` -> `Build, Execution, Deployment` -> `Toolchains`, create a new toolchain
3. Set CMake executable to `support/ides/clion/nix-cmake.sh` and the other elements of the
toolchain to the symlinks created in step 1.

Note it is important to set the symlink of ctest to be able to run unit-tests from CLion.