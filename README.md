# Pocket Engineer

A local engineering study workbench, built around a C++20 solver. Flat-color, responsive interfaces for Android and the web; no account, API key, remote font, or cloud calculation.

[Open the website — Render](https://pocket-engineer.onrender.com/) · [Downloads](https://github.com/MuhammadTahaBinZaeem/FOP-Project/releases) · [Builds and test artifacts](https://github.com/MuhammadTahaBinZaeem/FOP-Project/actions)

## What is implemented

The shared C++ catalog exposes 55 bounded problem types with runnable examples and explicit input contracts:

| Subject | Implemented families |
| --- | --- |
| Algebra | arithmetic, cancellation, quadratic factors, linear/quadratic equations, numeric trigonometry and logarithms |
| Calculus | polynomial differentiation, integration, definite integrals and tangents; selected removable limits and monomial curve analysis |
| Linear algebra | RREF, determinant, inverse, unique/dependent/inconsistent systems, multiplication, transpose, rank, real 2×2 eigenvalues, vectors |
| Differential equations | selected separable, linear, exact, Bernoulli, homogeneous and second-order families; exponential IVPs, Euler and RK4 |
| DLD | bases, signed addition, truth tables, canonical POS, exact 2–4 variable K-maps with don't-cares, selected combinational circuits and flip-flops |
| LCA / ENA | resistive DC nodal analysis, two-mesh circuits, dividers, source transformations, superposition, supplied Thévenin/Norton equivalents, maximum power, RC/RL step responses |
| Programming / units | bounded C++ teaching traces, branches, loops, arrays, functions, factorials; dimension-checked SI conversions |

This is **not a general-purpose solver for every problem in these courses**. No arbitrary C++ execution, OCR, general symbolic ODE solver, AC phasor analysis, larger eigensystems or arbitrary natural-language interpretation is claimed. The topic's “What this solver supports” panel states its limits. See [input reference](docs/INPUT_REFERENCE.md) and [curriculum scope](docs/CURRICULUM_COVERAGE.md).

## Website and Android

The website compiles the same engine to WebAssembly and runs it inside a Web Worker, keeping the interface responsive. Its service worker caches the interface **and solver**. Wait for **Ready offline** before disconnecting; then the same address can solve after a cold reload without a native server. Browser storage can still be evicted.

The Android application bundles the interface and calls native C++ through an asynchronous JNI bridge. It has no INTERNET permission. Trusted assets use an HTTPS-style local origin; arbitrary remote navigation and file access are blocked.

Both interfaces include subject search, explicit type selection, example inputs, numbered calculation steps, numerical-check evidence, warnings, sampled charts/K-map tables, and the last 30 inputs in device-only history. There are no SVG assets in the maintained website.

See [platform instructions and limitations](docs/PLATFORM_SUPPORT.md).

The generated pocket-and-circuit logo is a raster image, not a letter monogram. Its header asset is 1.8 KB; the critical offline bundle, including WASM, is approximately 745 KB. See [UI/Android stress evidence](docs/UI_ANDROID_STRESS.md) and [brand sources](design/brand/README.md).

## Build and test

Native desktop:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
./build/pocket-engineer-server 8080 www
```

Open http://127.0.0.1:8080. Without built WASM assets this local website uses the native server. CPack creates portable ZIP/TGZ distributions; packaged executables find the website relative to their installation.

Browser/WASM (Emscripten 5.0.7):

```sh
bash tools/build_web.sh
npm ci
npx playwright install chromium
npm test
```

Render builds `main` with `bash tools/build_web.sh` and publishes `www` as a static site. The script uses pinned Emscripten 5.0.7, enforces an offline bundle budget and versions the cache from asset contents. GitHub Pages remains a secondary tested preview; relative paths also support subdirectory hosting. GitHub Actions separately gates native and browser test artifacts.

## Test evidence: three different things

1. **Independent comparisons**: deterministic arithmetic, Leibniz determinants, adjugate inverses, constructed linear systems, circuit formulas, polynomial integrals, Boolean output replay, plus error cases. These are not expected answers produced by the engine being tested.
2. **Regression snapshots**: 5,000 easy + 5,000 medium + 5,000 hard rows for each of 55 topics = 825,000 rows. Replaying these catches changes, but does **not** establish independent correctness. Some bounded families have few distinct inputs; the generator reports uniqueness.
3. **Platform tests**: real browser interaction and offline reload, sanitizer runs, desktop builds, and Android emulator instrumentation. A phone-sized browser test is not an Android device test.

```sh
./build/pe-independent-tests docs/generated/INDEPENDENT_V3.json
./build/pe-independent-tests docs/generated/INDEPENDENT_STRESS_V3.json --stress
./build/pe-generate-tests test-data 5000
./build/pe-verify-corpus test-data 0 docs/generated/REGRESSION_V3.json
```

[Test history](docs/TEST_HISTORY.md) records results, failures and corrections. Legacy reports with fields named “correct” mean **snapshot matches**, not proven mathematical correctness. A solver verification label describes a method check; numerical sampling is not a proof for all inputs.

The deeper run passed **1,606,929 independent/edge checks**. The extended local browser run passed **24/24 tests**, including **800 endurance UI solves**: 600 independently expected answers and 200 intentional invalid inputs. Later CI passed the expanded 26-test suite; two additional responsive-chart checks passed locally. The actual offline APK was run locally: all 55 topic examples completed, and three runs matched 160 independently expected arithmetic answers in total. [Measured emulator jank and remaining limits](docs/UI_ANDROID_STRESS.md) are disclosed. This is measured coverage, not a universal “100% correct” claim.

## C++ ownership and efficiency

All solvers, mathematical parsing, classification, topic contracts, explanation steps, numerical checks and plot sampling are in C++. JavaScript handles presentation, caching, history and worker messaging; Kotlin hosts the native application.

`npm run audit:source` measures maintained production runtime source bytes, excluding tests, generators, the archived original application, third-party code and generated WASM glue. It reports the runtime C++ share **and a second percentage including HTML/CSS**, so the denominator is explicit. The runtime-code gate is 80%; markup and styles are not executable solver code.

Input lengths, recursion depth, matrix dimensions, ODE iteration counts, history size and chart samples are bounded. The browser worker has a watchdog. The desktop server binds only to loopback and validates Host/Origin, body sizes and canonical file paths. No generated training corpus is downloaded by the app.

## Explanation corpus

The separate C++ generator produces 550,000 generic teaching-text rows:

```sh
./build/pe-generate-explanations explanation-data 10000
```

These are template combinations for future editing, **not** 550,000 distinct mathematical derivations or proof of explanation quality. They are not loaded at runtime. See [explanation corpus guidance](docs/EXPLANATION_CORPUS.md).

## SemPPEC project summary

**Problem:** engineering students need a usable workbench when connectivity, device performance or budget is limited.

**Product:** choose a subject, enter its supported form, optionally ask for a type suggestion, then inspect a local C++ solution with working, assumptions and check evidence.

**Design:** a desktop sidebar becomes bottom navigation on phones; the same focused workbench uses flat colors, system fonts, raster illustrations, HTML tables and canvas plots.

**Boundary:** deterministic methods support the declared forms. The project does not claim “100% of engineering problems,” flawless explanations or validation on every device.
