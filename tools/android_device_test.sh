#!/usr/bin/env bash
# Keep diagnostics even when instrumentation fails, before CI stops the emulator.
set -uo pipefail
pe_test_status=0
pe_variant="${PE_ANDROID_TEST_TYPE:-debug}"
if [ "$pe_variant" = release ]; then pe_task=connectedReleaseAndroidTest; else pe_task=connectedDebugAndroidTest; fi
gradle -p android ":app:$pe_task" "-PpeTestBuildType=$pe_variant" --no-daemon \
  -Pandroid.injected.androidTest.leaveApksInstalledAfterRun=true || pe_test_status=$?
pe_report="android/app/build/reports/androidTests/$pe_variant-device"
mkdir -p "$pe_report/screenshots"
adb pull /sdcard/Android/data/com.pocketengineer.app/files/evidence "$pe_report/screenshots" || pe_test_status=1
# Instrumentation closes activities. Relaunch before collecting foreground
# diagnostics; otherwise dumpsys just reports 'No process found'.
adb shell am start -W -n com.pocketengineer.app/.MainActivity > "$pe_report/startup.txt"
adb shell dumpsys meminfo com.pocketengineer.app > "$pe_report/memory.txt"
adb shell dumpsys gfxinfo com.pocketengineer.app > "$pe_report/frames.txt"
adb logcat -d -t 300 > "$pe_report/logcat.txt"
exit "$pe_test_status"
