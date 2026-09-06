#!/usr/bin/env bash
# This script runs only inside the workflow's disposable Android emulator.
set -uo pipefail
pe_failed=0
bash tools/android_device_test.sh || pe_failed=1
# Release may use a different project certificate. Test it in a clean test
# installation; debug reports have already been pulled before removal.
adb uninstall com.pocketengineer.app || pe_failed=1
adb uninstall com.pocketengineer.app.test || true
PE_ANDROID_TEST_TYPE=release bash tools/android_device_test.sh || pe_failed=1
exit "$pe_failed"
