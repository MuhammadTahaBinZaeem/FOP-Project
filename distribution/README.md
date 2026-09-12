# Public binary mirror

Only tested release artifacts belong in versioned `vX.Y.Z-rcN` directories here. The source repository stays private; the existing Render static build copies these verified files to generated `www/downloads/` for anonymous delivery. No repository visibility change or runtime GitHub token is required.

Create a fresh version directory with `tools/stage_public_release.mjs`, then validate it with `tools/publish_download_mirror.mjs`. Every file except the checksum list itself must be listed in `SHA256SUMS.txt`. Do not replace published installer bytes under an existing version URL.

The mirror is excluded from PWA precaching, Android assets, CPack's embedded website and the website ZIP. CI includes a sentinel installer to verify exclusion. `verify-downloads.yml` fetches actual Render files without authentication and executes downloaded desktop packages; actual browser-button downloads and installed-APK tests are separate checks.

Binary files are delivery artifacts, not C++ source, and are not included in the maintained-source language ratio. Outbound downloads count against the existing Render workspace's bandwidth allowance; no new paid service or plan is created.
