# Pocket Engineer

A local engineering study workbench, built around a C++20 solver. Flat-color, responsive interfaces for Android and the web; no account, API key, remote font, or cloud calculation.

[Open the website — Render](https://pocket-engineer.onrender.com/) · [Public downloads](https://pocket-engineer.onrender.com/downloads/v0.5.0-rc2/RELEASE_NOTES.txt) · [Builds and test artifacts — repository access required](https://github.com/MuhammadTahaBinZaeem/FOP-Project/actions)

## What is implemented

The shared C++ catalog exposes 55 bounded problem types with runnable examples and explicit input contracts:

| Subject | Implemented families |
| --- | --- |
| Algebra | arithmetic, cancellation, quadratic factors, linear/quadratic equations, numeric trigonometry and logarithms |
| Calculus | polynomial differentiation, integration, definite integrals and tangents; selected removable limits and monomial curve analysis |
| Linear algebra | RREF, determinant, inverse, unique/dependent/inconsistent systems, multiplication, transpose, rank, real 2×2 eigenvalues, vectors |
| Differential equations | selected separable, linear, exact, Bernoulli, homogeneous and second-order families; exponential IVPs, Euler and RK4 |
| DLD | bases, signed addition, truth tables, canonical POS, clickable 2–6 variable K-maps with don't-cares/manual groups, selected combinational circuits and flip-flops; state-machine tables, reduction, simulation, diagrams and D-input/output equations |
| LCA / ENA | DC/AC RLC and controlled-source MNA, schematic editor, backward-Euler RLC transients, computed Thévenin/Norton ports, superposition, AC sweeps, component sensitivities; original divider/mesh/step-response topics |
| Signals and transforms | FFT/IFFT, convolution/correlation, waveform generation, analog/digital response, FIR/IIR difference equations, windowed-sinc FIR design, finite Z-transform and bounded inverse-Z/Laplace families |
| Programming / units | bounded C++ teaching traces, branches, loops, arrays, functions, factorials; dimension-checked SI conversions |

This is **not a general-purpose solver for every problem in these courses**. No arbitrary C++ execution, OCR, nonlinear/SPICE circuit model, general symbolic ODE solver, larger eigensystem or unrestricted natural-language interpretation is claimed. New engineering labs use validated forms and structured inputs, not guessed circuit topology. The topic's support panel states its limits. See [visual labs and engineering reference](docs/ENGINEERING_LABS_V5.md), [input reference](docs/INPUT_REFERENCE.md) and [curriculum scope](docs/CURRICULUM_COVERAGE.md).

## Website and Android

The website compiles the same engine to WebAssembly and runs it inside a Web Worker, keeping the interface responsive. Its service worker caches the interface **and solver**. Wait for **Ready offline** before disconnecting; then the same address can solve after a cold reload without a native server. Browser storage can still be evicted.

The Android application bundles the interface and calls native C++ through an asynchronous JNI bridge. It has no INTERNET permission. Trusted assets use an HTTPS-style local origin; arbitrary remote navigation and file access are blocked.

Use **Test demos · 825k cases** to choose subject, problem type and difficulty. All 165 compressed sets are preloaded for offline use: 5,000 easy, medium and hard cases per original topic. Only the selected set is decompressed in a separate worker; only 25 rows are rendered. Calibrate on your device for a measured time estimate, run a page or 5,000 cases, stop safely, and export expected/actual comparisons. These are stored regression snapshots, not independent proofs. New engineering labs have separate independent tests, not an invented 5,000-case bank for every new operation.

**Prepare / repair offline access** registers or repairs the offline worker, verifies SHA-256 content and includes all demo sets. Once **Ready offline** appears, revisit the same Render URL in the same browser without internet. A first-ever offline visit cannot work, and clearing/evicting site storage removes that installation. The complete download is about 9.2 MB; the critical app/engine is about 1.25 MB. Browser installation and persistent-storage grants depend on the browser.

Both interfaces include subject search, natural question input, automatic correction of clearly wrong type selections, a manual-mode switch, example inputs, numbered calculation steps, numerical-check evidence, warnings, sampled charts/K-map tables, and the last 30 inputs in device-only history. The interpretation is shown and the original text is preserved. See [natural input examples and boundaries](docs/NATURAL_INPUT.md). There are no SVG assets in the maintained website.

See [platform instructions and limitations](docs/PLATFORM_SUPPORT.md).

[0.5.0-rc2 Render downloads](https://pocket-engineer.onrender.com/downloads/v0.5.0-rc2/RELEASE_NOTES.txt)
include the optimized, non-debuggable, development-signed Android preview,
Windows/macOS desktop launchers, a static Linux ZIP that runs on NixOS, and the
complete WASM website ZIP, with checksums. Read the [prerelease notes](docs/releases/v0.5.0-rc2.md)
for installation, signing and device-validation limits. The website's “Get the
app” page has direct, labelled downloads for each platform.

The release also includes [all 825,000 exported UI comparisons](https://pocket-engineer.onrender.com/downloads/v0.5.0-rc2/PocketEngineer-0.5.0-demo-comparisons.tar.gz): input, expected/actual answers, verification labels, per-case timings, per-bank summaries and an export audit. These are stored-snapshot comparisons, not an independent oracle. The GitHub repository remains private; authenticated GitHub CI downloads alone do not establish anonymous public access. All five actual Render download buttons now pass anonymous browser downloads and SHA-256 checks; the downloaded packages pass Windows/macOS/Linux execution, NixOS relocation, four Android emulator tests and 82 website tests. [Public download validation](https://pocket-engineer.onrender.com/downloads/v0.5.0-rc2/PUBLIC_DOWNLOAD_VALIDATION.json).

The generated pocket-and-circuit logo is a raster image, not a letter monogram. Its header asset is 1.8 KB. The critical offline budget remains 1.5 MB, with a separate 12 MB limit including the demo bank. See [v5 implementation and validation](docs/ENGINEERING_LABS_V5.md), [previous hardening record](docs/JANK_INPUT_DOWNLOADS_V4.md), [UI/Android stress evidence](docs/UI_ANDROID_STRESS.md) and [brand sources](design/brand/README.md).

## Build and test

Native desktop:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
./build/pocket-engineer-server 8080 www
```

Open http://127.0.0.1:8080. Without built WASM assets this local website uses the native server. CPack creates ZIP/TGZ distributions with a `Start-Pocket-Engineer` launcher; packaged executables find the website relative to their installation. Linux release CI uses `-DPE_STATIC_LINUX=ON` to remove host C/C++ shared-runtime and ELF-loader requirements. The launcher opens a loopback-only server and your browser; no Python or Node is needed to run a desktop package.

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

The 0.5 run passes **1,606,929 independent/edge checks**, **4,112 natural-input checks**,
and all **825,000 regression snapshot comparisons**, plus **23,374 workbench** and
**31,941 engineering-study checks**. The browser suite passes **82/82 tests**,
including loading every demo bank, a hard RK4 selection regression and binary-download routing.
ASan/UBSan passes all five native suites. Windows, macOS, Linux and Android CI
pass; Android runs four instrumentation tests in each of Debug and Release.
These are distinct coverage categories, not one inflated independent total.
[Current tests, Android traces and download evidence](docs/TEST_HISTORY.md)
record failures and remaining limits; the [0.4 evidence](docs/JANK_INPUT_DOWNLOADS_V4.md)
is retained as history. No universal “100% correct” claim is made.

## C++ ownership and efficiency

All solvers, mathematical parsing, classification, topic contracts, explanation steps, numerical checks and plot sampling are in C++. JavaScript handles presentation, caching, history and worker messaging; Kotlin hosts the native application.

`npm run audit:source` measures maintained production runtime source bytes, excluding tests, generators, the archived original application, third-party code and generated WASM glue. Both gates are enforced: **80% C++ runtime and 77% including HTML/CSS**. Current values are approximately 82.12% and 77.10%, respectively; run the audit for exact bytes.

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
