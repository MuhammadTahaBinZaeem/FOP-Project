# Offline platform support and packages

Pocket Engineer keeps its solving, verification, corpus generation, and comparison logic in C++20. The browser page is only a local presentation layer; it never sends a problem to a cloud solver.

## Desktop packages

The CMake install package contains:

- pocket-engineer, the native CLI;
- pocket-engineer-server, the local HTTP server;
- the complete static website under share/pocket-engineer/www;
- corpus and explanation-data generators;
- corpus comparison tool and core documentation.

Build a package on each target operating system:

~~~
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
cpack --config build/CPackConfig.cmake -G ZIP -C Release
~~~

Run the downloaded package without an internet connection:

~~~
pocket-engineer-server 8080 share/pocket-engineer/www
~~~

Then open http://127.0.0.1:8080. The package needs no network permission or online API key. Windows uses WinSock, while Linux and macOS use POSIX sockets.

The GitHub Actions workflow builds, tests, smoke-compares, packages, and uploads a ZIP artifact on Ubuntu, Windows, and macOS after the repository is pushed. [Run 33416606640](https://github.com/MuhammadTahaBinZaeem/FOP-Project/actions/runs/33416606640) passed all three desktop package jobs. The Linux package is also built locally in this workspace; Windows and macOS binaries are produced by their corresponding GitHub runners because this Linux workspace does not contain those operating systems' toolchains.

## Android

The android directory is a Gradle Android application project with:

- a Kotlin WebView shell;
- the same static local webpage from www, bundled as application assets;
- a JNI bridge that calls the C++ engine directly; and
- no INTERNET permission.

The webpage detects the PocketEngineerAndroid bridge. On Android it calls native C++ directly; on desktop it calls the local C++ server. The application therefore remains offline in both cases.

Open android in Android Studio, install the Android SDK/NDK requested by the project, then assemble:

~~~
cd android
gradle :app:assembleRelease
~~~

The resulting APK is at android/app/build/outputs/apk/release. The project supports API 24 and newer. This workspace does not have the Android SDK, NDK, Java, or Gradle installed, so an APK cannot be honestly claimed as locally built here; however, [GitHub Actions run 33416606640](https://github.com/MuhammadTahaBinZaeem/FOP-Project/actions/runs/33416606640) compiled it with the pinned Android toolchain and uploaded the release APK artifact.

## PWA and web assets

The site includes a web manifest and service worker that cache the interface, JavaScript, CSS, manifest, and PNG illustrations after the first local server visit. It contains no SVG files, CDN requests, remote fonts, or external runtime dependencies.

For a fully functional desktop web install, keep the packaged native server running; the service worker preserves the interface assets, while the local server supplies the C++ solver API.
