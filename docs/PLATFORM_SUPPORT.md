# Platforms, offline operation and downloads

## Website / PWA

The primary published site is https://pocket-engineer.onrender.com/. GitHub Pages remains a secondary preview at https://muhammadtahabinzaeem.github.io/FOP-Project/.

A Web Worker loads an Emscripten build of the C++ engine. The service worker caches a versioned, bounded list including engine.js and engine.wasm. No solve request goes to a remote API. Assets use relative URLs for subpath deployment.

1. Open the site once online.
2. Open “Get the app” and wait for “Ready offline.”
3. Install with Chrome/Edge's install option, or use Android Chrome's Install app / Add to home screen.
4. Disconnect and reopen the same address. The browser test suite exercises this cold reload and a fresh solve.

A standalone desktop PWA can be installed using Chrome/Edge on Windows, macOS or Linux. Modern Firefox can run the website but does not offer the same desktop installation UI. Private mode, storage eviction, user-cleared site data, restrictive browser policies, or insufficient space can remove/prevent offline availability. First-ever use requires the solver download. Do not open index.html as a file:// page.

Requirements: JavaScript, WebAssembly, Web Workers, and HTTPS or localhost for service workers. The C++ browser heap has a 128 MiB maximum and grows on demand; that is a safety cap, not a measured total browser memory footprint. Main-thread UI work remains small, and graph data is sampled.

## Native Android

The Gradle application supports Android API 24+ with an updated Android System WebView. ABI builds use the NDK; Java/Kotlin source targets JDK 17. The app has no INTERNET permission and bundles the C++ solver and website, so it works on first launch without a prior website visit.

Security/lifecycle:

- WebViewAssetLoader serves trusted assets at an HTTPS-style local origin.
- File/content access and mixed content are disabled.
- Third-party subresources are blocked, and allowed GitHub links open outside the WebView.
- The JNI boundary uses standard UTF-8 byte arrays, not Modified UTF-8.
- Calculation runs on a single native-work executor and returns through a JSON-safe callback.
- Insets, WebView pause/resume/destruction and back navigation are handled.

Build with Android Studio, or the pinned SDK/NDK/CMake/Gradle versions in the workflow:

```sh
gradle -p android :app:assembleDebug :app:assembleRelease
gradle -p android :app:connectedDebugAndroidTest
```

The **debug APK is installable and development-signed**. By default a local
Release build is unsigned until an owner-controlled keystore is supplied. CI
explicitly opts into a development-signed, optimized **Release preview** when
that key is absent. It is non-debuggable, but is not a production-signed release
or a stable update identity. See [signing setup](ANDROID_SIGNING.md). Never commit
a private signing key. Replacing a differently signed preview requires removing
the old installation, which clears its local history; export important solutions
first. No automatic uninstall is performed on a user's device.

Instrumentation launches the real WebView, waits for the JNI engine, solves
arithmetic and an inconsistent linear system, then recreates the activity and
checks local history. Touch tests enter text using native key events, check the
native clipboard, rotate, check back navigation, and correct a wrong topic from
a naturally worded determinant question. Both Debug and Release are tested with
separate reports, screenshots and foreground diagnostics. The manifest's absent
INTERNET permission makes a remote solver unavailable during these tests.

Local API 35 emulator runs additionally exercised all 55 topic examples, repeated expected-answer checks, real keyboard entry, native JSON saving and print preview. [The evidence report](UI_ANDROID_STRESS.md) distinguishes native touch injection, CDP-driven WebView controls, smoke examples and independent expected answers. It also records remaining emulator jank; these are not low-end physical phone benchmarks.

A build or emulator pass is not evidence that every old physical Android device performs well. No low-end physical phone benchmark is claimed.

## Native Windows / macOS / Linux

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
cpack --config build/CPackConfig.cmake -G ZIP -C Release
```

The package contains native CLI/server executables, the website, corpus tools
and documentation. Extract the entire ZIP, then open its `Start-Pocket-Engineer`
launcher (`.cmd`, `.command` or `.sh`). It opens the default browser; if browser
launch is unavailable, visit the printed 127.0.0.1 address. Keep the terminal
running. No Python or Node runtime is required. Port 8765 is preferred, with an
available-port fallback; browser history belongs to that address and port.
It serves only loopback, not a public production endpoint. The website uses
native HTTP fallback if WASM assets are not included.

Windows and macOS binaries are compiled by their respective GitHub runners. GitHub Actions also uploads a self-contained website artifact after compiling the browser engine. Download links in the UI distinguish release assets from development build artifacts.

The historical 0.3.0 Linux ZIP required an Ubuntu-style ELF loader, glibc 2.38 and
GLIBCXX_3.4.31, and failed to launch unmodified on NixOS. **0.4.0 Linux CI packages
statically link the C/C++ runtime** and require no dynamic ELF loader. The
downloaded Ubuntu-built artifact was run unmodified on NixOS and passed CLI,
local-server, website and natural-input checks. This is tested x86_64 Linux
portability, not a promise for every kernel/architecture. Public-release checks
are recorded separately in the release notes. Windows/macOS executables are
still not code-signed/notarized; do not disable system-wide security protections.

## Test interpretation

See [TEST_HISTORY.md](TEST_HISTORY.md) and the [0.4 hardening record](JANK_INPUT_DOWNLOADS_V4.md)
for completed runs. An earlier build is not proof that a changed application
passed; consult the source commit and run attached to each result.

Technical basis: [Android local-content guidance](https://developer.android.com/develop/ui/views/layout/webapps/load-local-content), [Emscripten modular output](https://emscripten.org/docs/compiling/Modularized-Output.html), and [C++/JavaScript interoperation](https://emscripten.org/docs/porting/connecting_cpp_and_javascript/Interacting-with-code.html).
