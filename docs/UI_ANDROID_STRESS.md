# UI and Android hardening — 2026-09-06

This is a test record, not a claim of universal device compatibility or universal mathematical coverage. Earlier reports remain historical evidence. Current source and commands are the reproducible reference.

## Changes and regression coverage

- Removed overlapping hero caption positioning. Wrapped headings and labels, constrained grid children, increased interactive targets to at least 44 CSS pixels, and stacked mobile inputs. Header now carries the generated pocket-and-circuit logo, also used by PWA and Android launcher icons.
- Compressed the artwork and moved original PNGs outside `www`. The critical offline bundle is approximately 745 KB uncompressed, including the 622 KB C++ WASM solver. `tools/verify_web_assets.mjs` enforces a 1.5 MB budget and derives the service-worker cache version from asset contents.
- Disabled input/type changes during a solve, reset stale copy feedback, bounded native-server fetch time, and handled installation failure feedback. Native Android back returns from secondary views to the workbench; JSON export survives activity recreation and reports a missing document picker.
- Added actual dropdown/example/solve interactions for all 55 catalog topics, in addition to lower-level WASM tests. Layout checks cover 320, 360, 390, 667 landscape, 768, 1024 and 1440 pixel widths; all four navigation views; control containment, sibling overlap and touch-target sizes. Enlarged text gets a separate overflow check.
- Clipboard and JSON downloads are exercised for real. Browser print and installation event handlers are simulated because an incognito headless context cannot confirm OS print or PWA installation. They must not be reported as real installation tests.

## Deeper native checks

`./build/pe-independent-tests docs/generated/INDEPENDENT_STRESS_V3.json --stress` completed **1,606,929 checks with 0 failures**. It covers all 65,536 four-variable Boolean functions (truth-table equivalence of emitted expressions), all 6,561 three-variable disjoint ON/don't-care assignments, and 50,000 iterations of selected numerical families. Oracles use separate determinant formulas, constructed systems, adjugates, circuit ratios and Boolean evaluation—not stored answers from the engine itself. It does not prove minimal literal count for every K-map or independently validate every curriculum topic.

The separate 825,000-row corpus remains a snapshot consistency check. `CORPUS_UNIQUENESS_V3.json` reports actual distinct inputs; finite Boolean/adder/factorial spaces cannot truthfully yield 5,000 distinct values for every difficulty. Repeated rows are not disguised as unique problems.

## Reproduce UI endurance

```sh
npm ci
bash tools/build_web.sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
PE_STRESS_ROUNDS=100 npm test
```

On NixOS, set `PE_BROWSER_PATH` to a locally working Chromium/Brave executable. Every endurance round performs four actual UI solves: arithmetic, a constructed linear system, a K-map, and malformed arithmetic. Expected answers are independently calculated in the test. Halfway through, disconnect networking and cold-reload the app. Reports contain request counts, page errors, per-round wall time and browser process metrics. Those metrics exclude the worker heap and are not physical Android measurements.

## Android evidence boundary

The earlier `ed1eb18` CI build ran an API 35 emulator successfully, but its test mainly drove JavaScript inside the WebView. The newer test uses real Android touch injection, accessibility text entry, native clipboard inspection, portrait/landscape screenshots and back navigation. Debug builds enable adb WebView inspection; release builds do not. CI uploads screenshots, instrumentation reports, memory and frame diagnostics.

Local APK/emulator execution and the new test run are still being verified; do not infer a successful run from the presence of test code. Physical low-memory ARM phones, OEM WebView variations, release signing, OS document-picker completion and actual PWA installation remain separate validation work.

## Hosting

The user selected Render's “My Workspace”. The website is a static site with C++ compiled to WASM from the deployed Git commit using pinned Emscripten 5.0.7. No paid solver server, database, or cloud computation is required. GitHub Pages remains a secondary preview while Render is brought online.

Build references: [Render static sites](https://render.com/docs/static-sites), [Emscripten SDK installation](https://emscripten.org/docs/getting_started/downloads.html). Local emulator references: [Nixpkgs Android composition](https://github.com/NixOS/nixpkgs/blob/master/doc/languages-frameworks/android.section.md), [Android emulator command line](https://developer.android.com/studio/run/emulator-commandline).
