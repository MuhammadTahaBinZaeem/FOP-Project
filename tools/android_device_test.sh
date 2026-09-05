#!/usr/bin/env bash
# Keep diagnostics even when instrumentation fails, before CI stops the emulator.
set -uo pipefail
pe_test_status=0
gradle -p android :app:connectedDebugAndroidTest --no-daemon \
  -Pandroid.injected.androidTest.leaveApksInstalledAfterRun=true || pe_test_status=$?
mkdir -p android/app/build/reports/androidTests/screenshots
adb pull /sdcard/Android/data/com.pocketengineer.app/files/evidence android/app/build/reports/androidTests/screenshots || pe_test_status=1
# Instrumentation closes activities. Relaunch before collecting foreground
# diagnostics; otherwise dumpsys just reports 'No process found'.
adb shell am start -W -n com.pocketengineer.app/.MainActivity > android/app/build/reports/androidTests/startup.txt
adb shell dumpsys meminfo com.pocketengineer.app > android/app/build/reports/androidTests/memory.txt
adb shell dumpsys gfxinfo com.pocketengineer.app > android/app/build/reports/androidTests/frames.txt
adb logcat -d -t 300 > android/app/build/reports/androidTests/logcat.txt
exit "$pe_test_status"
