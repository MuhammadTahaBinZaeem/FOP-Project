# Test history and result comparison

This file separates independent correctness comparisons, same-engine regression snapshots, and platform execution. Re-running answers produced by the same engine cannot independently establish mathematical correctness. Historical “correct” counts below mean snapshot matches.

## 2026-09-06 — responsive UI, branding and deeper stress

Commit `6e42a81`: local browser suite 24/24 passed (800 endurance UI solves plus all 55 topics through actual controls in both layouts). Native deeper checks: 1,606,929 passed, 0 failed. Full snapshot replay: 825,000 matching, 0 mismatching. Rebuilt ASan/UBSan CTests: 2/2 passed in 16.17 seconds. Seven viewport sizes, native touch tests and Render deployment are documented with explicit evidence boundaries in [UI_ANDROID_STRESS.md](UI_ANDROID_STRESS.md).

The image preparation tool initially selected sharp 0.34.5. `npm audit` detected a high-severity inherited libvips advisory; the dependency was updated to pinned 0.35.4 before processing final assets. The resulting dependency audit reported zero vulnerabilities. No untrusted image upload processor is exposed by the app.

The new touch-driven Android test in CI run 33988785167 failed opening the Subjects tab, although the existing JavaScript-driven native solve test passed. WebView padding was replaced with a padded native parent so fixed HTML navigation uses the unobscured viewport. Failure screenshots/logs are now retained even when instrumentation fails. The correction requires its own rerun; it is not marked passed based on source changes alone.

The first maximum-input browser test expected an equals sign in an RK4 answer that correctly uses “≈”; both viewport tests failed in the test parser with NaN. The actual result was `y(10) ≈ 1.10517091808`, matching `exp(0.1)`. Corrected the parser, retaining the independent numerical comparison and asserting the output format.

The next boundary run also assumed the scientific calculator accepted the global 4096-byte request maximum. Its deliberately stricter expression-module limit is 512 bytes. The test now solves an exactly 512-byte expression, checks rejection of a 4095-byte input, then verifies recovery with `2+2`.

Render's first cold deployment succeeded, but a subsequent auto-deploy restored a partial `build-emsdk` cache with no `emsdk` launcher. Checking only directory existence incorrectly skipped SDK source setup. The build now checks the launcher and restores source from the pinned official SDK commit while preserving downloaded compiler data. This fixes the observed missing-file error; both a rebuilt deployment and a later cache-reuse deployment must be checked.

Android touch tests passed after the native-parent inset fix in runs 33989208751 and 33989237051. The first evidence download revealed that Gradle uninstalled the app before screenshots were pulled; its memory file said “No process found”, so it is not a memory benchmark. The test wrapper now retains test APKs using the [AndroidX test configuration](https://github.com/androidx/androidx/blob/androidx-main/gradle.properties), requires screenshot retrieval and relaunches the foreground app before collecting diagnostics.

## 2026-09-06 — 0.3.0 workbench and offline engine

Completed during development:

- Fresh baseline c1b7685: native Release build and original CTest passed.
- First independent suite: 98,235 checks, 16 failures from exact string expectations for near-zero linear-system outputs. Changed the oracle to compare numerical values at a declared tolerance; did not suppress tiny valid output values.
- Re-run: 98,235/98,235 checks passed. Added polynomial comparisons afterward; the current machine-readable run is [INDEPENDENT_V3.json](generated/INDEPENDENT_V3.json).
- AddressSanitizer + UndefinedBehaviorSanitizer: both CTest suites passed, 11.10 seconds total on the development machine. Compiler emitted GCC standard-library regex warnings; no sanitizer finding was reported in the exercised paths.
- First browser subset: 6/6 passed in desktop and 390×844 phone viewports using native-server fallback (selection, matrix rank cases, history, injection, keyboard and JSON export). These are not WASM-offline or Android-device results.
- Full WASM browser checks and changed Android emulator build are separate workflow gates; their results must be recorded after they complete.
- CI run 33987351873 initially failed Android dependency validation because the newly used AndroidX WebView loader required `android.useAndroidX=true`. Added the missing Gradle property; the failure was a real configuration defect, not ignored.
- Full local C++/WASM browser suite: 14/14 passed in 24.0 seconds, including offline cold reload and a fresh divider solve at both viewport sizes. No HTTP solver fallback was allowed in the WebAssembly checks.
- Expanded independent suite: 99,326/99,326 checks passed. The regenerated 825,000-row corpus replayed with 825,000 answer and verification-state matches; repeated rows are disclosed by the generator, and this is still snapshot consistency, not 825,000 independent oracles.

Fixed defects include unary-minus/power precedence, scientific notation formatting, non-finite arithmetic, recursive/iteration input bounds, signed and unsigned overflow, JSON escape handling, K-map don't-care dominance ties, empty ON sets, incompatible unit dimensions, milli/mega case, inconsistent/dependent linear systems, and actual inverse/system residual checks. Euler/RK4 results outside reference tolerance now disclose that state rather than claiming verified accuracy.

Reproduce the new independent suite with `./build/pe-independent-tests`, and browser checks with `npm test` after building WASM. Browser screenshots and structured results are uploaded with the workflow. No physical low-end Android benchmark or universal “100% correctness” claim is made.

## 2026-08-31 — release candidate 0.2.0

### Commands

~~~
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -DPE_ENABLE_IPO=OFF
cmake --build build-release -j2
ctest --test-dir build-release --output-on-failure
./build-release/pe-verify-corpus test-data 0 docs/generated/CORPUS_COMPARISON.json
./build-release/pe-generate-explanations explanation-data 10000
~~~

### Results

| Check | Result |
| --- | --- |
| C++20 release build | passed |
| CTest engine suite | 1 / 1 passed, 0 failed |
| Golden inputs compared | 825,000 |
| Answer text matches | 825,000 correct; 0 incorrect |
| Verification-status matches | 825,000 correct; 0 incorrect |
| Solver errors during comparison | 0 |
| Overall answer-and-verification matches | 825,000 / 825,000 |
| Release comparison elapsed time | 12.221 seconds |
| Throughput | about 67,500 cases/second |
| Generated explanation rows | 550,000 |
| Explanation-data size | about 180 MB |
| SVG assets in www | 0 |
| Linux offline ZIP package | generated and contents inspected |

The machine-readable per-topic comparison is [CORPUS_COMPARISON.json](generated/CORPUS_COMPARISON.json). Every one of the 55 declared topics has 15,000 cases and reports 15,000 answer matches, 15,000 verification matches, and zero solver errors.

The generated Linux package was PocketEngineer-0.2.0-Linux.zip with SHA-256 f3c17857116ad04c723ab72ba3e1bacc70bc28ab0125f076a873a18ae01fbcbc. Its ZIP contents include the native CLI, local server, corpus tools, website, PNG assets, comparison report, and platform documentation.

### Cross-platform release validation

[GitHub Actions run 33416606640](https://github.com/MuhammadTahaBinZaeem/FOP-Project/actions/runs/33416606640) passed on source commit 377b24d:

| Delivery | CI checks and published artifact |
| --- | --- |
| Ubuntu | C++ build, CTest, generated smoke corpus comparison, ZIP package: pocket-engineer-ubuntu-latest |
| Windows | MSVC build, CTest, generated smoke corpus comparison, ZIP package: pocket-engineer-windows-latest |
| macOS | C++ build, CTest, generated smoke corpus comparison, ZIP package: pocket-engineer-macos-latest |
| Android API 24+ | pinned SDK/NDK/CMake build of the offline WebView/JNI app and release APK: pocket-engineer-android-apk |

The Android artifact is a real release APK compiled against the same C++ engine, not a mock frontend. The desktop ZIP artifacts contain the local server and static site for offline use.

Expected verifier distribution in the comparison:

| Verification kind | Cases |
| --- | ---: |
| verified_exact | 495,000 |
| verified_exhaustive | 75,000 |
| verified_numerical | 255,000 |

### Platform checks

| Platform path | Current evidence |
| --- | --- |
| Linux desktop | release build, CTest, full corpus comparison, local server/API, and CPack package tested in this workspace |
| Windows desktop | portable WinSock server and CMake/CPack path are in source; GitHub Actions builds/tests/packages on windows-latest after push |
| macOS desktop | POSIX server and CMake/CPack path are in source; GitHub Actions builds/tests/packages on macos-latest after push |
| Android API 24+ | offline WebView/JNI project is included; local SDK/NDK/Gradle are absent in this workspace, so no APK is falsely recorded as built |

### Comparison semantics

A case counts as correct only if:

1. the native solver returns success;
2. its answer text exactly matches the golden expected answer; and
3. its verification status exactly matches the expected status.

The comparison tool retains up to ten mismatch examples in its JSON report if any result fails. The current report has an empty mismatch list.

## Ongoing regression procedure

After a solver, parser, or explanation change:

1. Run CTest.
2. Regenerate the corpus only if the public solver contract intentionally changed.
3. Run the full comparison and commit the new small JSON report.
4. Inspect any mismatch examples before accepting an answer change.
5. Let GitHub Actions repeat a smoke generation/comparison and produce Windows, macOS, and Linux packages.
