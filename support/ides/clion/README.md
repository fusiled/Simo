# Use Nix packages in CLion

From the terminal, open a development environment with `nix develop`.

Inside the development environment, launch CLion.

On macOS, this can be done with `open -na "CLion.app"`.

One issue is that the integrated terminal may complain about `bash: bind: command not found`
and show escape symbol. The solution is to run `nix develop` in the terminal.