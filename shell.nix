{ pkgs ? import <nixpkgs> {} }:
with pkgs;
mkShell {
    buildInputs = [
        gcc
        gnumake
        xorg.libX11
        xorg.libXrandr
        libnotify
        pkg-config
        clang-tools
    ];
}
