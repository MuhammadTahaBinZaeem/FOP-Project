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

Local browser result: **24/24 passed in 2.7 minutes** at commit `6e42a81`, with `PE_STRESS_ROUNDS=100`. The endurance subset performed **800 actual UI solves**, comprising 600 independent expected-answer checks and 200 intentional failures. No page errors were recorded. Average four-solve rounds were 510 ms desktop / 479 ms mobile-layout on this development machine, including UI automation. Foreground JS heap at completion was 7.7 MB / 10.2 MB before forced garbage collection; these are not total process memory or worker-heap measurements. Machine-readable reports and unedited screenshots are under [evidence/2026-09-06](evidence/2026-09-06).

The rebuilt AddressSanitizer/UndefinedBehaviorSanitizer suite passed both CTests in 16.17 seconds, with no sanitizer finding. Its flags were explicitly confirmed in the generated compiler flags file; the project does not have a `PE_ENABLE_SANITIZERS` CMake option. To reproduce, configure with `-DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer -O1'`.

The earlier `ed1eb18` CI build ran an API 35 emulator successfully, but its test mainly drove JavaScript inside the WebView. The newer test uses real Android touch injection, accessibility text entry, native clipboard inspection, portrait/landscape screenshots and back navigation. Debug builds enable adb WebView inspection; release builds do not. CI uploads screenshots, instrumentation reports, memory and frame diagnostics.

The APK was installed and run **locally on this NixOS machine** in an Android 15/API 35 x86_64 AVD, emulator 36.6.11, WebView 124.0.6367.219. KVM was active; Wi-Fi and mobile data were disabled. Although 1536 MB was initially requested, the emulator raised guest RAM to **2560 MB**; there is no 1.5 GB-device claim. Two guest CPU cores were used. The unaccelerated-graphics cold app launch reported 1361 ms; the host-graphics run reported 863 ms. These are individual debug/emulator observations, not a controlled physical-device startup benchmark.

Native Android touch injection, actual keyboard entry, copy feedback, opening the document picker and opening print preview were exercised locally. A JSON save was completed through Android's real document picker; [the exported JSON](evidence/2026-09-06/android-software/saved-solution.json) contains input `100*(3+7)` and answer `1000`. The print preview was visually inspected and displayed the solution; no physical print job is claimed. The screenshot review found light status-bar icons on a light background; the native theme now explicitly requests dark system-bar icons.

| Local debug APK run | Expected arithmetic answers | UI solve median / p95 | Main process PSS after run | Reported janky frames |
| --- | ---: | ---: | ---: | ---: |
| SwiftShader software graphics | 100/100 | 679 / 1134 ms | 97,053 KiB | 1365/1862 (73.31%) |
| Host graphics (Mesa Intel UHD) | 30/30 | 282 / 545 ms | 94,270 KiB | 221/695 (31.80%) |
| Host graphics, system-bar fix (`247c703`) | 30/30 + 55 catalog smoke examples | 290 / 739 ms | 97,001 KiB | 526/1596 (32.96%) |

All three runs had zero page errors. Timings include automation, scrolling and keyboard/layout transitions, not just C++ computation. The first host-graphics comparison used the same app build and two guest CPU cores but a shorter run; it is evidence that graphics configuration materially affects these observations, not proof of smooth rendering everywhere. **Jank remains a performance concern in this emulator**, even with host graphics. Main-process PSS excludes separate renderer/system overhead. Physical low-memory ARM phones, OEM WebView variations, production release signing, and actual browser PWA installation remain separate validation work. Raw measurements and screenshots are linked in the evidence directory; no bad result was relabeled as a pass.

The third run's [home](evidence/2026-09-06/android-final/home.png), [K-map](evidence/2026-09-06/android-final/kmap.png) and [RK4](evidence/2026-09-06/android-final/rk4.png) screenshots were visually inspected. The system-bar icon contrast fix is visible. This run used the APK from `247c703`; `8825bfc` changed only the accessibility wait in the test. All 55 catalog examples exercised actual dropdown/example/solve controls through WebView/JNI, but are smoke checks, not 55 independent oracles. The 30 repeated arithmetic answers have separate expected values.

Visual review also exposed distorted graph labels: a fixed 900×280 bitmap was stretched to a narrow, taller CSS box. The chart now draws in CSS-pixel coordinates with a device-pixel backing store capped at 2×, measures label margins, and redraws on resize. Its observer is disconnected on replacement. Two new browser regressions check font/backing-store dimensions across phone, landscape, desktop and hidden-view navigation. They passed locally (2/2); an APK rebuild is required to validate that change on Android.

Run [33990241649](https://github.com/MuhammadTahaBinZaeem/FOP-Project/actions/runs/33990241649) on `8825bfc` passed Windows, macOS, Linux and real-touch Android instrumentation. Its [landscape screenshot](evidence/2026-09-06/android-ci/android-landscape.png) was inspected after waiting for rotation to settle. Website run [33990241648](https://github.com/MuhammadTahaBinZaeem/FOP-Project/actions/runs/33990241648) passed all 26 then-current browser tests. Those CI runs use the default shorter endurance setting; the separate 800-solve local run above is not attributed to CI.

After the chart correction, the full local suite on `de21cf4` passed **28/28 in 3.5 minutes** with `PE_STRESS_ROUNDS=100`: another 800 UI solves (600 independent expected answers, 200 deliberately invalid inputs), zero page errors. The average four-solve rounds were 706 ms desktop / 667 ms phone-layout. These automation wall times are not a controlled speed comparison with the earlier run. Reports and the inspected, correctly proportioned chart screenshot are in [browser-de21cf4](evidence/2026-09-06/browser-de21cf4). Website CI [33990843528](https://github.com/MuhammadTahaBinZaeem/FOP-Project/actions/runs/33990843528) also passed all 28 tests with its shorter default endurance setting.

Package CI [33990843529](https://github.com/MuhammadTahaBinZaeem/FOP-Project/actions/runs/33990843529) on `de21cf4` passed all three desktop platforms and both real Android instrumentation tests. Installing that rebuilt debug APK over the earlier CI build was rejected with `INSTALL_FAILED_UPDATE_INCOMPATIBLE` because the development signing keys differed. Only the disposable emulator's test-app installation was removed and replaced; committed reports and the exported JSON were retained. This is why the download notes do not promise a stable production update identity.

The rebuilt `de21cf4` APK then completed **all 55 catalog examples plus 10/10 independently expected arithmetic answers locally**, with zero page errors and networking disabled. The test checked the actual Android canvas backing dimensions and label font; [the corrected chart screenshot](evidence/2026-09-06/android-de21cf4/rk4.png) and [home screenshot](evidence/2026-09-06/android-de21cf4/home.png) were visually inspected. Raw [run evidence](evidence/2026-09-06/android-de21cf4/report.json) is retained. Main-process PSS was 97,592 KiB and reported jank was 445/1385 frames (32.13%), so the earlier emulator-performance caveat still applies. Across the four local runs, 170 arithmetic expected-answer comparisons passed; repeated catalog runs are not counted as additional unique topic coverage.

To repeat local APK testing on an explicitly selected disposable emulator:

```sh
PE_ANDROID_SERIAL=emulator-5556 PE_ANDROID_ROUNDS=100 PE_ANDROID_CATALOG=1 \
  PE_ANDROID_RENDERER=host node tools/android_ui_check.cjs
```

Install and launch the debug APK first. The script does not choose or clear a physical phone. The native-touch portion uses adb input; the endurance/catalog portion drives actual WebView controls via Playwright/CDP. It opens the native document and print dialogs but does not claim to complete their OS operations. The separately saved JSON linked above was completed manually.

APK size comparison: `ed1eb18` debug APK = 16,797,239 bytes; `11db5af` debug APK = 8,587,254 bytes (48.9% smaller), chiefly from removing original oversized PNGs from the bundle. These are development-signed universal APKs, not production-signed releases.

## Hosting

The user selected Render's “My Workspace”. [Pocket Engineer is live on Render](https://pocket-engineer.onrender.com/), service `srv-dae79mon74is73cm11ug`. Initial deployment `dep-dae79n8n74is73cm13fg` built commit `6e42a81` and reached `live`. The homepage and WASM returned HTTP 200; WASM has the correct `application/wasm` MIME type; the error-log scan was empty. C++ is compiled to WASM from the deployed Git commit using pinned Emscripten 5.0.7. No paid solver server, database, or cloud computation is required. GitHub Pages remains a secondary preview.

The partial-cache rebuild defect was corrected in `d157e23`. Deployment `dep-dae7ff49v7es73av8cu0` then reached `live`, and the subsequent cache-reuse deployment `dep-dae7gouq1p3s73cs0qq0` of `1b4c031` also reached `live`. Hosted-site checks passed: cold offline reload, all 55 examples through actual UI controls, clipboard/JSON export and navigation (3/3 tests).

The chart-fix deployment `dep-dae7sp67bikc73d652k0` of `de21cf4` reached `live`. Four checks against the actual Render URL passed in 23.9 seconds: cold offline reload/fresh solve, all 55 catalog examples, responsive chart labels, and copy/export/navigation/installation-handler feedback. The served `app.js` SHA-256 exactly matched both the source checkout and downloadable website artifact. Print/install callbacks remain simulated in this headless browser check.

Build references: [Render static sites](https://render.com/docs/static-sites), [Emscripten SDK installation](https://emscripten.org/docs/getting_started/downloads.html). Local emulator references: [Nixpkgs Android composition](https://github.com/NixOS/nixpkgs/blob/master/doc/languages-frameworks/android.section.md), [Android emulator command line](https://developer.android.com/studio/run/emulator-commandline).
