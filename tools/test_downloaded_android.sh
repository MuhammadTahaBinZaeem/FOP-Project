#!/usr/bin/env bash
# Run the matching CI instrumentation APK against an actual downloaded app APK.
# Requires an explicitly selected disposable emulator; never uninstalls user data.
set -euo pipefail
pe_apk="${1:?Provide downloaded APK}"
pe_test_apk="${2:?Provide matching instrumentation APK}"
pe_report="${3:?Provide a new evidence directory}"
pe_adb="${PE_ADB:-adb}"
pe_serial="${PE_ANDROID_SERIAL:?Select your disposable emulator}"
case "$pe_serial" in emulator-[0-9]*) ;; *) echo 'Only an explicit emulator is allowed' >&2; exit 2;; esac
test -f "$pe_apk"
test -f "$pe_test_apk"
mkdir "$pe_report"
"$pe_adb" -s "$pe_serial" get-state
# A certificate mismatch fails here instead of silently deleting app history.
"$pe_adb" -s "$pe_serial" install -r "$pe_apk" | tee "$pe_report/install.txt"
"$pe_adb" -s "$pe_serial" install -r "$pe_test_apk" | tee "$pe_report/install-test.txt"
"$pe_adb" -s "$pe_serial" shell svc wifi disable
"$pe_adb" -s "$pe_serial" shell svc data disable
pe_status=0
"$pe_adb" -s "$pe_serial" shell am instrument -w -r \
  com.pocketengineer.app.test/androidx.test.runner.AndroidJUnitRunner \
  | tee "$pe_report/instrumentation.txt" || pe_status=1
"$pe_adb" -s "$pe_serial" pull \
  /sdcard/Android/data/com.pocketengineer.app/files/evidence "$pe_report/screenshots" || pe_status=1
"$pe_adb" -s "$pe_serial" shell am start -W -n com.pocketengineer.app/.MainActivity > "$pe_report/startup.txt"
"$pe_adb" -s "$pe_serial" shell dumpsys package com.pocketengineer.app > "$pe_report/package.txt"
"$pe_adb" -s "$pe_serial" shell dumpsys meminfo com.pocketengineer.app > "$pe_report/memory.txt"
"$pe_adb" -s "$pe_serial" shell dumpsys webviewupdate > "$pe_report/webview.txt"
"$pe_adb" -s "$pe_serial" logcat -d -t 500 > "$pe_report/logcat.txt"
# `am instrument` can return exit status 0 even when JUnit failed.
grep -q 'OK (4 tests)' "$pe_report/instrumentation.txt" || pe_status=1
if grep -qE 'FAILURES!!!|INSTRUMENTATION_FAILED|Process crashed' "$pe_report/instrumentation.txt"; then pe_status=1; fi
exit "$pe_status"
