# Test history and result comparison

This file separates independent correctness comparisons, same-engine regression snapshots, and platform execution. Re-running answers produced by the same engine cannot independently establish mathematical correctness. Historical “correct” counts below mean snapshot matches.

## 2026-09-09–10 — offline repair, visual editors and engineering labs

Local release-candidate validation before platform packaging:

| Test | Result | Evidence |
| --- | --- | --- |
| Release CTest | All 5 suites passed | [log](evidence/2026-09-10/ctest-final.log) |
| ASan + UBSan Debug | All 5 suites passed | [log](evidence/2026-09-10/sanitizer-tests.log) |
| Original independent stress | 1,606,929 checked, zero failures | [report](evidence/2026-09-10/independent-stress-v5.json) |
| Original regression corpus | 825,000 answers and verification labels match | [report](evidence/2026-09-10/regression-v5.json) |
| New workbench | 12,770 checked, zero failures | [log](evidence/2026-09-10/workbench-tests.log) |
| New circuit/signal studies | 31,941 checked, zero failures | [repaired log](evidence/2026-09-10/engineering-studies-fixed.log) |
| Actual desktop/mobile-layout browser interactions | 68/68 passed | [log](evidence/2026-09-10/browser-release-candidate.log) |
| Full 5,000-case UI run, desktop | 1,576 ms, zero mismatches, no >50 ms main-thread tasks observed | [report](evidence/2026-09-10/demo-endurance-desktop.json) |
| Full 5,000-case UI run, phone layout | 1,491 ms, zero mismatches, no >50 ms main-thread tasks observed | [report](evidence/2026-09-10/demo-endurance-android-layout.json) |
| Maintained runtime C++ share | 82.457%; 77.387% including HTML/CSS | [exact denominator](evidence/2026-09-10/source-final.json) |

These timings are from this desktop host, not a physical Android phone. The UI runs exercise all 5,000 records of one selected easy bank in each layout; native regression replay separately covers all 165 banks. Expanded browser tests also cover all 13 signal forms, circuit studies, six-variable K-maps, draft preservation, PNG/JSON download events, offline reload/repair, maximum FFT output and bounded failures. [Circuit](evidence/2026-09-10/lab-circuit-320.png), [signals](evidence/2026-09-10/lab-signals-320.png), [state editor](evidence/2026-09-10/lab-fsm-320.png) and [FFT result](evidence/2026-09-10/fft-result-android-layout.png) are actual narrow-browser screenshots.

Failures are retained, not discarded: [offline baseline](evidence/2026-09-10/offline-before.log) reproduced missing-cache/registration defects; [initial engineering run](evidence/2026-09-10/engineering-studies-tests.log) caught two superposition residual-normalization false negatives, fixed with a scale-aware floor. The first broad UI run had 56/58 passes because its touch-target test measured the checkbox glyph instead of its clickable label; the corrected test still enforces 44-pixel target height. A subsequent version-specific download-link test was generalized to validate versioned release URLs while keeping the previous tested downloads live until new packages are published.

The [v5 implementation reference](ENGINEERING_LABS_V5.md) documents algorithms, contracts, exclusions and performance safeguards. New Android instrumentation is defined but package/installed-APK results are reported separately after execution. Production signing and physical-device smoothness are not implied by these local results.

## 2026-09-07–08 — natural input, jank investigation and downloaded packages

Shared C++ interpretation now handles documented human wording and corrects
clearly mistaken topic selections. The raw question remains in the editor,
history and export; the normalized interpretation is visible. Unsupported or
ambiguous constraints produce clarification instead of guessed answers.

- Native CTest: 3/3; natural-input checks: 4,112, zero failures.
- Independent stress: 1,606,929 checks, zero failures. Regression snapshots:
  825,000 matching answers/verification states. These are not equivalent counts.
- ASan/UBSan: 3/3 suites. Extended browser test: 34/34, including 400 endurance
  solves (300 independently expected answers, 100 intentional rejections).
- [Package run 34261473560](https://github.com/MuhammadTahaBinZaeem/FOP-Project/actions/runs/34261473560)
  passed Windows, macOS, Linux and 3/3 Android tests in each of Debug and Release.
  [Website run 34261473717](https://github.com/MuhammadTahaBinZaeem/FOP-Project/actions/runs/34261473717)
  passed all 34 browser tests. Source: `de7fa4e`.
- [Published-download run 34262496068](https://github.com/MuhammadTahaBinZaeem/FOP-Project/actions/runs/34262496068)
  downloaded the actual 0.4.0-rc1 ZIPs and checksums and executed their CLI/server
  on Windows x64, Ubuntu x64 and macOS arm64. All three passed. The universal
  macOS archive contains both slices; this run executed arm64, not Intel.
- The public Linux ZIP also passed unmodified on NixOS in a relocated path with
  spaces, resolving the observed 0.3 Ubuntu-loader failure. Checks include
  expected CLI answers, bundled UI, wrong-type recovery and cross-origin denial.
- The public APK installed on the explicitly selected API 35 x86_64 emulator
  with Wi-Fi/data disabled. Three consecutive instrumentation runs each passed
  3/3: real typing/touches, solution/clipboard, rotation/back navigation,
  wrong-selection recovery, recreation/history and native JNI solving. The
  installed package is version 0.4.0, non-debuggable, development-signed.
- All five live Render download buttons were clicked in an actual Chromium
  browser; all five downloads matched the published SHA-256 manifest. This is
  an actual download test, not just checking an HTTP link or archive name.
- The public website ZIP was extracted, served with the downloaded static
  server, and passed 6/6 natural-input tests including offline cold reload and
  fresh WASM solves. Native desktop packages need their localhost server running.
- The downloaded Linux ZIP's actual launcher was executed directly on NixOS
  from outside the extracted folder; four browser natural-input/manual-mode
  checks passed through its packaged native server at localhost:8765.
- The direct-download UI revision `1b0360c` passed
  [website run 34262624967](https://github.com/MuhammadTahaBinZaeem/FOP-Project/actions/runs/34262624967)
  (34/34) and [all-platform package run 34262624978](https://github.com/MuhammadTahaBinZaeem/FOP-Project/actions/runs/34262624978),
  including six Android Debug/Release tests. Render deployment
  `dep-dag54l8ae00c738f9k90` reached live.

The local published Release APK's isolated ten-swipe journeys reported 6/261
(2.30%), 4/219 (1.83%) and 6/240 (2.50%) janky frames. All three had a 31 ms frame
median; legacy jank counters remained 60.15–75.83%. **This is not a 60 fps result
or a controlled reduction from the older combined approximately 32% figure.**
The CI emulator's Release scroll reported 86.51%, and prior local debug/plain
control experiments also performed poorly. All are retained. Real phones and
16KB-page runtime devices remain untested; production signing/notarization remain
open. See [hardening details and failure history](JANK_INPUT_DOWNLOADS_V4.md).

Earlier Android runs failed because of transient accessibility lookup/IME
coordinates, then literal shell quotes injected by UiAutomation. Exact text
assertions, rendering synchronization and correctly encoded native key injection
fixed the tests; failures were not relabelled as passing builds. The failed
software-renderer experiment with a System UI ANR was rejected, not counted as
smooth. The local pre-update disposable emulator's test data was backed up before
replacing its differently signed installation; no physical device was cleared.

Evidence: [download checks](evidence/2026-09-08/public-downloads),
[published APK repeats](evidence/2026-09-08/public-apk),
[release notes](releases/v0.4.0-rc1.md), [input grammar](NATURAL_INPUT.md).

## 2026-09-06 — responsive UI, branding and deeper stress

Commit `6e42a81`: local browser suite 24/24 passed (800 endurance UI solves plus all 55 topics through actual controls in both layouts). Native deeper checks: 1,606,929 passed, 0 failed. Full snapshot replay: 825,000 matching, 0 mismatching. Rebuilt ASan/UBSan CTests: 2/2 passed in 16.17 seconds. Seven viewport sizes, native touch tests and Render deployment are documented with explicit evidence boundaries in [UI_ANDROID_STRESS.md](UI_ANDROID_STRESS.md).

The image preparation tool initially selected sharp 0.34.5. `npm audit` detected a high-severity inherited libvips advisory; the dependency was updated to pinned 0.35.4 before processing final assets. The resulting dependency audit reported zero vulnerabilities. No untrusted image upload processor is exposed by the app.

The new touch-driven Android test in CI run 33988785167 failed opening the Subjects tab, although the existing JavaScript-driven native solve test passed. WebView padding was replaced with a padded native parent so fixed HTML navigation uses the unobscured viewport. Failure screenshots/logs are now retained even when instrumentation fails. The correction requires its own rerun; it is not marked passed based on source changes alone.

The first maximum-input browser test expected an equals sign in an RK4 answer that correctly uses “≈”; both viewport tests failed in the test parser with NaN. The actual result was `y(10) ≈ 1.10517091808`, matching `exp(0.1)`. Corrected the parser, retaining the independent numerical comparison and asserting the output format.

The next boundary run also assumed the scientific calculator accepted the global 4096-byte request maximum. Its deliberately stricter expression-module limit is 512 bytes. The test now solves an exactly 512-byte expression, checks rejection of a 4095-byte input, then verifies recovery with `2+2`.

Render's first cold deployment succeeded, but a subsequent auto-deploy restored a partial `build-emsdk` cache with no `emsdk` launcher. Checking only directory existence incorrectly skipped SDK source setup. The build now checks the launcher and restores source from the pinned official SDK commit while preserving downloaded compiler data. This fixes the observed missing-file error; both a rebuilt deployment and a later cache-reuse deployment must be checked.

Android touch tests passed after the native-parent inset fix in runs 33989208751 and 33989237051. The first evidence download revealed that Gradle uninstalled the app before screenshots were pulled; its memory file said “No process found”, so it is not a memory benchmark. The test wrapper now retains test APKs using the [AndroidX test configuration](https://github.com/androidx/androidx/blob/androidx-main/gradle.properties), requires screenshot retrieval and relaunches the foreground app before collecting diagnostics.

Run 33989562232 passed and retained the real Android screenshots. A later run, 33989958245, hit an asynchronous accessibility-tree race: immediate `findObject` did not yet see the input after navigation/keyboard reflow. The test now waits for the workbench and polls for the actual native EditText, recording a screenshot if it remains unavailable. This does not replace the real touch event with JavaScript.

The rerun on `8825bfc` passed: package run 33990241649 (Windows, universal macOS, Linux, Android real-touch instrumentation) and website run 33990241648 (26 browser tests). Render deployment `dep-dae7n449v7es73avfbc0` of that commit reached `live`.

The APK was also installed and exercised locally in an API 35 x86_64 emulator with connectivity disabled. Three runs matched 100, 30 and 30 independently expected arithmetic answers, respectively; the last also solved all 55 catalog examples through UI controls. These are different coverage categories, not 215 independent topic checks. Actual JSON saving and print preview were inspected. Software rendering showed 73.31% reported jank; host rendering showed 31.80% and 32.96%. This is a remaining performance concern, not a silently discarded failure. Detailed environment, screenshots, timing and memory scope are in [UI_ANDROID_STRESS.md](UI_ANDROID_STRESS.md).

Android screenshot review caught squeezed chart labels caused by mismatched canvas and CSS aspect ratios. Responsive, bounded-resolution drawing replaced the fixed bitmap dimensions; two new resize/navigation regression tests passed locally. The screenshot that exposed the issue is retained, not replaced with a fabricated clean result.

Download inspection: the Ubuntu-built native executable from `de21cf4` cannot launch directly on this NixOS host because its generic ELF loader is not available. `readelf` also identified glibc 2.38 and GLIBCXX_3.4.31 requirements. The release notes now disclose that distribution boundary and point NixOS users to the local source build or website/PWA; this failure is not reported as a successful cross-distribution package run.

Chart-fix rerun on `de21cf4`: the full local browser suite passed 28/28 in 3.5 minutes, including 800 actual UI solves (600 independent expected answers plus 200 rejected invalid inputs), with zero page errors. CI website run 33990843528 passed all 28 tests at the default shorter endurance count. Both old and new evidence are retained separately.

Package run 33990843529 passed Windows, universal macOS, Linux and both Android instrumentation tests on `de21cf4`. The final local native CTest rerun passed 2/2 in 1.05 seconds, and `npm audit` reported zero vulnerabilities. Four actual Render-site checks passed in 23.9 seconds; the deployed and packaged `app.js` hashes matched the source checkout.

Release validation: SHA-256 checks passed for every staged asset; the four ZIP distributions passed archive integrity checks. Info-ZIP's general-purpose APK check complained about extra-field entries on native libraries. Android SDK `apksigner verify --verbose` verified the APK's v2 signature, `zipalign -c -p 4` passed, and Android successfully installed and launched it. The APK is 8,587,630 bytes; the self-contained website ZIP is 326,354 bytes. The packaging helper was also checked to reject an existing output directory without overwriting its contents.

Final APK run on `de21cf4`: all 55 catalog examples plus 10/10 independent arithmetic answers passed locally with connectivity disabled and zero page errors. The responsive chart dimensions/font were asserted inside the actual Android WebView and the corrected screenshot was inspected. Main-process PSS was 97,592 KiB, reported jank 32.13%; no physical-device smoothness claim is made. Development signing keys differed from the earlier CI build, requiring replacement of only the disposable emulator's test installation; previous evidence remained intact.

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
