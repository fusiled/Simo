#  Copyright 2026 Matteo Fusi and Contributors
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
{
  description = "Nix flake for building and testing the CMake-based Simo project";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-26.05";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
        simo = pkgs.stdenv.mkDerivation {
          pname = "simo";
          version = "0.0.1";
          src = ./.;

          nativeBuildInputs = with pkgs; [
            cmake
            ninja
            doxygen
          ];

          buildInputs = with pkgs; [
            boost
            glaze
          ];

          cmakeFlags = [
            "-DPORTABLE_BUILD=ON"
            "-DENABLE_RELEASE_LTO=OFF"
            "-DCMAKE_CXX_SCAN_FOR_MODULES=OFF"
          ];

          doCheck = true;
          checkPhase = ''
            ctest --output-on-failure
          '';
        };
      in {
        packages.default = simo;
        checks.default = simo;

        devShells.default = pkgs.mkShell.override {
            stdenv = pkgs.clangStdenv;
        } {
          packages = with pkgs; [
            boost
            cmake
            ninja
            clang-tools
            llvm
            glaze
            doxygen
            ast-grep
          ];
        };
      }
    );
}
