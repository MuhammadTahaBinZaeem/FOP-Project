# Minimal, isolated local Android test environment; no system configuration changes.
{ pkgs ? import <nixpkgs> { config = { allowUnfree = true; android_sdk.accept_license = true; }; } }:
(pkgs.androidenv.composeAndroidPackages {
  platformVersions = [ "35" ];
  buildToolsVersions = [ "35.0.0" ];
  includeEmulator = true;
  includeSystemImages = true;
  systemImageTypes = [ "default" ];
  abiVersions = [ "x86_64" ];
  includeNDK = false;
}).androidsdk
