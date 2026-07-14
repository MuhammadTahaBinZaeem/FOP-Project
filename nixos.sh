#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_FILE="$SCRIPT_DIR/project_gui.cpp"
CORE_FILE="$SCRIPT_DIR/project.cpp"
OUTPUT_DIR="$SCRIPT_DIR/out/nixos"
EXE_FILE="$OUTPUT_DIR/project_gui.exe"
WINE_PREFIX_DIR="$OUTPUT_DIR/wine-prefix"

if ! command -v nix-shell >/dev/null 2>&1; then
  echo "nix-shell is required to build the NixOS version." >&2
  exit 1
fi

mkdir -p "$OUTPUT_DIR"

needs_build=0
if [ ! -f "$EXE_FILE" ] || [ "$SOURCE_FILE" -nt "$EXE_FILE" ] || [ "$CORE_FILE" -nt "$EXE_FILE" ]; then
  needs_build=1
fi

if [ "$needs_build" -eq 1 ]; then
  shell_command=$(cat <<EOF
set -e
if command -v x86_64-w64-mingw32-g++ >/dev/null 2>&1; then
  compiler=x86_64-w64-mingw32-g++
else
  compiler=x86_64-w64-mingw32-c++
fi
"\$compiler" -std=c++17 -O2 -Wall -Wextra -static-libgcc -static-libstdc++ -mwindows "$SOURCE_FILE" -o "$EXE_FILE"
if command -v wine64 >/dev/null 2>&1; then
  wine_bin=wine64
elif command -v wine >/dev/null 2>&1; then
  wine_bin=wine
else
  echo "wine is required to run the GUI on NixOS." >&2
  exit 1
fi
mkdir -p "$WINE_PREFIX_DIR"
WINEPREFIX="$WINE_PREFIX_DIR" "\$wine_bin" "$EXE_FILE"
EOF
)

  nix-shell --pure --quiet --expr 'let pkgs = import <nixpkgs> {}; in pkgs.mkShell { buildInputs = [ pkgs.wineWowPackages.full pkgs.pkgsCross.mingwW64.stdenv.cc ]; }' --run "$shell_command"
  exit 0
fi

shell_command=$(cat <<EOF
set -e
if command -v wine64 >/dev/null 2>&1; then
  wine_bin=wine64
elif command -v wine >/dev/null 2>&1; then
  wine_bin=wine
else
  echo "wine is required to run the GUI on NixOS." >&2
  exit 1
fi
mkdir -p "$WINE_PREFIX_DIR"
WINEPREFIX="$WINE_PREFIX_DIR" "\$wine_bin" "$EXE_FILE"
EOF
)

nix-shell --pure --quiet --expr 'let pkgs = import <nixpkgs> {}; in pkgs.mkShell { buildInputs = [ pkgs.wineWowPackages.full ]; }' --run "$shell_command"