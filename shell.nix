# Development shell for NixOS: `nix-shell`. Lean itself comes from elan,
# pinned by lean-toolchain.
{ pkgs ? import <nixpkgs> { } }:
pkgs.mkShell {
  packages = with pkgs; [ elan cmake gcc python3 ];
}
