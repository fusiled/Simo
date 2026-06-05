{
  description = "Dev shell with Simo available";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-26.05";
    flake-utils.url = "github:numtide/flake-utils";

    simo.url = "../../..";
    simo.inputs.nixpkgs.follows = "nixpkgs";
    simo.inputs.flake-utils.follows = "flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils, simo }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
      in
      {
        devShells.default = pkgs.mkShell.override {
          stdenv = pkgs.clangStdenv;
        } {
          packages = with pkgs; [
            boost
            cmake
            ninja
            llvm
            glaze
            doxygen

            simo.packages.${system}.default
          ];
        };
      });
}