#!/usr/bin/env bash
# Render static build: compile the engine from this exact checkout, not a CDN binary.
set -euo pipefail
cd "$(dirname "$0")/.."
if ! command -v emcmake >/dev/null 2>&1; then
  # Render can restore cached subdirectories without the SDK launcher/source.
  # Test the required file, not merely the directory. Extraction also recovers
  # that partial-cache state without deleting already downloaded compiler data.
  if [ ! -f build-emsdk/emsdk ]; then
    mkdir -p build-emsdk
    curl --fail --location --retry 3 --max-time 120 \
      https://codeload.github.com/emscripten-core/emsdk/tar.gz/41190c21c662e9cc1962aea94e71cbae9fd2fc87 \
      --output build-emsdk-source.tar.gz
    tar -xzf build-emsdk-source.tar.gz --strip-components=1 -C build-emsdk
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
