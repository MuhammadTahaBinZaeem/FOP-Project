// Record actual post-publication evidence without rebuilding any installer.
// Run once after copying raw reports into the tracked evidence directory.
import {readFile,writeFile} from 'node:fs/promises';
import {createHash} from 'node:crypto';
import assert from 'node:assert/strict';
const evidence='docs/evidence/2026-09-12/public-render-rc2';
const release='distribution/v0.5.0-rc2';
const name='PUBLIC_DOWNLOAD_VALIDATION.json';
const hash=data=>createHash('sha256').update(data).digest('hex');
const json=async file=>JSON.parse(await readFile(file,'utf8'));
const manifest=await readFile(`${release}/SHA256SUMS.txt`,'utf8');
const sums=new Map();
for(const line of manifest.trim().split('\n')){
  const match=line.match(/^([a-f0-9]{64})  ([A-Za-z0-9_.-]+)$/);
  assert(match&&!sums.has(match[2]),'Invalid manifest');
  sums.set(match[2],match[1]);
  assert.equal(hash(await readFile(`${release}/${match[2]}`)),match[1]);
}
assert(!sums.has(name),'Validation is already published; do not overwrite it');
const downloads=await json(`${evidence}/browser-downloads.json`);
assert.equal(downloads.downloads.length,5);
for(const file of downloads.downloads){
  assert.equal(file.actual_browser_click,true);assert.equal(file.checksum_verified,true);
  assert.equal(file.sha256,sums.get(file.name));
}
const desktop={};
for(const platform of ['windows','macos','ubuntu','nixos']){
  const report=await json(`${evidence}/desktop/${platform}.json`);
  for(const key of ['published_checksum_verified','cli','relocatable_server','bundled_ui','natural_input','wrong_type_correction','cross_origin_rejected','engineering_extension','offline_demo_bank'])assert.equal(report[key],true,`${platform}: ${key}`);
  assert.equal(report.sha256,sums.get(report.archive));
  delete report.localhost_url;desktop[platform]=report;
}
const ci=await json('docs/evidence/2026-09-12/anonymous-render-desktop-ci.json');
assert.equal(ci.conclusion,'success');assert.equal(ci.jobs.length,3);
assert(ci.jobs.every(job=>job.conclusion==='success'));
const android=await readFile(`${evidence}/android/instrumentation.txt`,'utf8');
assert.match(android,/OK \(4 tests\)/);assert.match(android,/Time: 49\.329/);
assert.match(await readFile(`${evidence}/android/install.txt`,'utf8'),/Success/);
assert.match(await readFile(`${evidence}/website-tests.log`,'utf8'),/82 passed \(2\.0m\)/);
const restart=await json(`${evidence}/offline-restart.json`);
const upgrade=await json(`${evidence}/cache-upgrade.json`);
assert.equal(restart.success,true);assert.equal(upgrade.success,true);
delete upgrade.profile;
const supplements=await json(`${evidence}/supplement-downloads.json`);
assert.equal(supplements.success,true);assert.equal(supplements.downloads.length,4);
for(const file of supplements.downloads){assert.equal(file.anonymous,true);assert.equal(file.sha256,sums.get(file.name));}
const report={
  release:'v0.5.0-rc2',build_source:'81feb663ea870a998de948cfc368f1aaf564361a',
  site:downloads.site,checked_at:supplements.checked_at,
  scope:'Actual anonymous downloads from the existing Render site; GitHub source visibility remains private. This supplement leaves all tested installer/archive bytes unchanged.',
  browser_downloads:downloads.downloads,downloaded_desktop_execution:desktop,
  anonymous_desktop_ci:{url:ci.url,workflow_source:ci.headSha,passed_jobs:3},
  downloaded_android:{passed_tests:4,elapsed_seconds:49.329,device:'API35 x86_64 emulator',network:'Wi-Fi and mobile data disabled',build_type:'Release, non-debuggable, development-signed',apk_sha256:sums.get('PocketEngineer-0.5.0-android-preview.apk')},
  downloaded_website:{passed_browser_tests:82,server:'Unmodified publicly downloaded Linux server on NixOS',website_sha256:sums.get('PocketEngineer-0.5.0-offline-website.zip')},
  live_offline_restart:restart,cached_install_update:upgrade,supplement_downloads:supplements.downloads,
  regression_scope:'825000 stored cases across 55 topics, 3 difficulties, 5000 cases per bank. All exported expected/actual records audited against hashed banks. Repeated runs are not new unique or independently proven answers.',
  limitations:[
    'Physical ARM-phone smoothness and 16 KB Android runtime-device execution are not established.',
    'Android is development-signed; desktop signing and macOS notarization remain open. macOS execution used arm64, not an Intel Mac.',
    'Emulator frame traces retain stalls; no universal zero-lag or 60 FPS guarantee.',
    'Offline revisits require an installation completed online in the same browser, with retained cache. First-ever offline visits and evicted storage cannot work.',
    'Question preservation is tested across the app-update action, not as unsolved-draft autosave across every ordinary refresh. The initial over-broad harness failure is retained in the detailed history.',
    'The original evidence ZIP records the build-validation checkpoint; this JSON records later public-download checks separately.',
    'Source percentages cover maintained runtime code, including HTML/CSS for the 77 percent gate; they exclude generated artifacts, test data, tests and documentation consistently.'
  ]
};
const data=JSON.stringify(report,null,2)+'\n';
await writeFile(`${release}/${name}`,data,{flag:'wx'});
await writeFile(`${release}/SHA256SUMS.txt`,manifest.trimEnd()+'\n'+hash(data)+'  '+name+'\n');
console.log(JSON.stringify({release:report.release,verified_existing_files:sums.size,added:name,sha256:hash(data)}));
