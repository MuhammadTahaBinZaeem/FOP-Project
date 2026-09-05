#!/usr/bin/env bash
# Render static build: compile the engine from this exact checkout, not a CDN binary.
set -euo pipefail
cd "$(dirname "$0")/.."
if ! command -v emcmake >/dev/null 2>&1; then
  if [ ! -d build-emsdk ]; then
    git clone --depth 1 --branch 5.0.7 https://github.com/emscripten-core/emsdk.git build-emsdk
  fi
  ./build-emsdk/emsdk install 5.0.7
  ./build-emsdk/emsdk activate 5.0.7
  # The upstream environment script tolerates unset optional variables.
  set +u
  source ./build-emsdk/emsdk_env.sh
  set -u
fi
emcmake cmake -S . -B build-wasm -DCMAKE_BUILD_TYPE=Release -DPE_ENABLE_IPO=OFF
cmake --build build-wasm --parallel 2
cp build-wasm/web/engine.js build-wasm/web/engine.wasm www/
node tools/verify_web_assets.mjs
