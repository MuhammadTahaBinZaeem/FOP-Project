# 0.4 hardening record — 2026-09-07

## Jank investigated first

The previous approximately 32% combined `gfxinfo` result was not treated as a mathematical-engine timing. `tools/android_frame_profile.cjs` now brackets idle, scrolling, navigation, solving and keyboard journeys separately. It records native frame diagnostics alongside WebView rAF cadence, long tasks and layout/script time. rAF is not proof that a frame was displayed. [Android's performance guidance](https://developer.android.com/topic/performance/measuring-performance) recommends measuring particular journeys and warns that a single aggregate rate obscures the work involved.

On this API 35, 1080×1920, two-vCPU, 2560 MB emulator, even a plain text page temporarily substituted into the same WebView showed 70–84% native jank in different graphics experiments and approximately 33 ms scrolling rAF intervals, with zero app layout work and no long JS tasks. Switching HWUI to Vulkan or disabling emulator Vulkan/host vblank did not eliminate it. Original OpenGL scroll frames spent a median 26 ms between issuing draw commands and swapping buffers; app input/traversal work was around 3 ms. This is evidence of an emulator/graphics-path bottleneck, not proof that every real phone has the same problem. No result is relabeled as a physical-device benchmark.

App-side corrections remove forced keyboard opening on example/topic selection, animated long-page scrolling during IME transitions, button color animation and an unnecessary image filter. Offscreen subject cards defer their content layout and fixed navigation has layout/paint containment. An initial intrinsic-size shorthand also imposed a minimum inline width and failed the 320px overflow test; it was replaced with block-axis-only intrinsic sizing and `min-width:0`. The corrected seven-size layout regression passed in both browser projects; the focused suite passed 8/8 including all 55 examples, exports and charts.

The initial full browser run containing the faulty shorthand was interrupted after its failures; it is not a passing run. A/B profiles that intercept local JS/CSS in the debug APK are explicitly marked as experiments, not rebuilt-APK tests. Some experiments overlapped browser test activity on the host; do not use their wall times as a controlled percentage speedup. Raw reports are retained in [evidence/2026-09-07/jank](evidence/2026-09-07/jank). Final rebuilt-APK validation is separate.

## Natural question input

`src/input.cpp` implements on-device interpretation, explicit-operation precedence,
bounded normalization, named-parameter validation and transparent type correction.
The UI retains the original question and offers a manual switch. Polynomial
equations now accept parentheses and expressions on both sides; expansion is not
misinterpreted as root finding. Named linear equations are converted to an
augmented matrix with an explicit variable order. See [NATURAL_INPUT.md](NATURAL_INPUT.md).

Initial native tests caught a legacy linear-equation fast path incorrectly
claiming an equation with `x` on the right, and a missing “tangent line to” filler.
Both were corrected. All 55 catalog examples preserve their manual-mode answers.
The new native suite also uses independently specified arithmetic/equation
families and rejection cases. The browser suite grew from 28 to 34 tests with
wrong-selection recovery, original-input preservation, manual mode and offline
natural-input journeys. Its first run found an obsolete “Suggested:” assertion
after identification changed to “Recognized:”; the assertion was updated to
check both that message and the actually selected equation type.

## Download changes awaiting published-package validation

Desktop ZIPs now contain `START_HERE.txt` and OS-specific launchers. The native
server chooses a stable localhost port, falls back when occupied, opens the
default browser and never binds to the LAN. Linux CI statically links the C/C++
runtime and checks absence of `INTERP`/`NEEDED` entries. Extracted-package tests
execute the actual packaged CLI/server, exercise corrected input through HTTP,
check relocated website discovery, and reject cross-origin requests.

Android release builds can use a project keystore supplied via GitHub secrets.
Until a permanent key is approved/configured, CI explicitly labels its fallback
as development-signed. The downloadable APK uses optimized Release native code
with debugging off, not the diagnostic Debug configuration. Previous development
certificates can differ, so in-place upgrades from old previews are not promised.

The APK's single JNI library statically owns libc++, uses 16KB ELF alignment,
and is checked with `zipalign -P 16` and an ELF-header validator. This follows
[Android's native page-size guidance](https://developer.android.com/guide/practices/page-sizes).
Alignment checks alone do not replace a 16KB device runtime test.
Physical-phone validation and Apple/Windows code-signing are still separate work.

## Completed local checks before release CI

- Native CTest: 3/3 suites passed.
- Natural-input suite: 4,112 checks, zero failures; 55 are catalog auto/manual
  equivalence checks, the rest have explicit success/failure/answer expectations.
- Independent stress: 1,606,929 checks, zero failures; timing p50 11.767 µs,
  p95 65.890 µs in this run, not an Android timing claim.
- Browser rerun: 34/34 passed in 1.6 minutes, including real controls and offline
  reload. The earlier 32/34 run with the obsolete message assertion is not counted
  as passing.
- Locally produced, extracted desktop ZIP: CLI expected answer, relocatable
  server, bundled UI, wrong-type correction and cross-origin rejection passed.
  This is not yet a public-download test or a static Ubuntu-to-NixOS test.

See [INDEPENDENT_STRESS_V4.json](generated/INDEPENDENT_STRESS_V4.json) and
[SOURCE_SHARE_V4.json](generated/SOURCE_SHARE_V4.json). Public-download and actual
new-APK results are recorded separately after those steps complete.

The extended 50-round-per-layout browser run passed 34/34 in 1.9 minutes:
400 endurance UI solves, comprising 300 independent expected answers and 100
intentional rejection cases. All 825,000 existing regression snapshots still
match answers and verification states. ASan/UBSan CTests passed 3/3 in 16.82 s.

The Ubuntu-built static Linux ZIP from CI run 34063775917 was downloaded as an
artifact and executed **unmodified on NixOS**. Its CLI, server, bundled UI,
natural-input correction and cross-origin checks passed. This resolves the old
generic ELF-loader failure for that tested x86_64 package; public-release download
validation remains a separate gate.

Android CI run 34063775917 failed the new keyboard-input test (the two existing
tests passed): the accessibility input was unavailable after a scroll-and-touch.
The harness now waits two rendering callbacks and Android idle before measuring
touch coordinates, asserts that the actual touch focused the editor, and saves a
failure screenshot if the EditText is still unavailable. It continues to inject
a real UiDevice touch and use the Android accessibility editor, not a DOM click
or a programmatic input assignment. The failed APK is not a verified release.

The next run still failed the accessibility lookup even though a separate
assertion confirmed that the actual touch had focused the editor. Typing is now
performed with Android Ctrl+A/Delete/text key events, with an exact editor-value
assertion before solving. Failure screenshots also include the accessibility
hierarchy. Both Debug and Release tests run even if one fails, with separate
diagnostics and clean test installations between different signing identities.

## 2026-09-08 continuation

Run 34064469956 failed two keyboard tests in each Android variant. The retained
screenshots show literal apostrophes surrounding the complete typed question.
Unlike adb's shell, UiAutomation's command execution does not consume shell
quotes. The harness now passes the space-encoded question as one unquoted
argument; it still requires exact text equality before pressing Solve. This was
a test injection defect, not an excuse to skip the real keyboard journey.

The confirmed-scroll local profile on the rebuilt 84da66a Debug APK measured
149/149 janky app scroll frames and 170/172 janky plain-control frames (98.84%).
Both pages actually moved; the profiler now asserts scroll displacement so that
a blocking system overlay cannot yield a misleading zero-frame pass. App scroll
layout work was 0.00364 seconds, with no recorded long JavaScript tasks.
These emulator results do **not** establish an app smoothness improvement. A
SwiftShader experiment showed a System UI ANR dialog and was discarded as an
invalid interaction run, not counted as a smooth result. Host graphics was
restored. Real-device and representative release-performance validation remain
open; a static alignment check is not a 16KB-page runtime test.

After restoring host graphics, the local Debug APK completed 50/50 independently
expected arithmetic answers and all 55 catalog examples through actual
WebView/JNI controls, with zero page errors. Native touches, keyboard entry,
clipboard, document-picker opening, print-preview opening and back navigation
also passed. The solution and responsive RK4 chart screenshots were inspected.
This functional run used 84da66a, not the final published release; it is not a
production-signing or physical-phone benchmark.

The fixed source `de7fa4eb1d49208338dd447b790b358fe432c57b` passed
[package CI 34261473560](https://github.com/MuhammadTahaBinZaeem/FOP-Project/actions/runs/34261473560)
on all three desktop platforms and Android (3/3 Debug and 3/3 Release tests).
[Website CI 34261473717](https://github.com/MuhammadTahaBinZaeem/FOP-Project/actions/runs/34261473717)
passed 34/34 browser tests. Live Render natural-input/offline checks passed 6/6.
The release APK's natural-question and landscape screenshots were inspected.

CI release scroll diagnostics still report 186/215 janky frames (86.51%);
Debug reports 188/222 (84.68%) in that software-rendered CI environment. Release
foreground app-process PSS was 58,375 KiB, excluding separately accounted shared
renderer/system processes. None of these measurements demonstrate physical-phone
smoothness or a causal Debug-to-Release speedup. See
[release evidence](evidence/2026-09-08/android-release-ci).

[0.4.0-rc1](https://github.com/MuhammadTahaBinZaeem/FOP-Project/releases/tag/v0.4.0-rc1)
was published from those unchanged artifacts. All five packages and source
metadata were downloaded from the release and matched SHA-256 checksums.
The published Linux ZIP passed unmodified on NixOS from a relocated path with
spaces. Public-download execution on the other desktop OSes and downloaded-APK
instrumentation are separate post-publication checks, recorded below when done.

The website now has five direct platform download buttons, per-platform launch
instructions, signing/upgrade warnings and a checksum link. The modified local
browser suite passed 34/34 in 56.7 seconds. The release's embedded website remains
the tested de7fa4e version; the deployed site's download-page additions are a
separate presentation-only change and do not rewrite published binaries.
