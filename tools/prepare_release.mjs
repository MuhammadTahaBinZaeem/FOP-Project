// Package already-tested GitHub artifacts without rebuilding or changing binaries.
// Usage: node tools/prepare_release.mjs ARTIFACTS WEBSITE OUTPUT SOURCE_COMMIT
import {access,copyFile,mkdir,readFile,readdir,stat,writeFile} from 'node:fs/promises';
import {createHash} from 'node:crypto';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
const [artifacts,website,output,commit]=process.argv.slice(2);
if(!artifacts||!website||!output||!/^[0-9a-f]{40}$/.test(commit||''))throw new Error('Provide artifact, website and new output directories plus a full tested source commit');
const target=path.resolve(output);
try{await access(target);throw new Error('Output already exists; choose a new directory to preserve existing assets');}catch(e){if(e.code!=='ENOENT')throw e;}
const signing=(await readFile(path.join(artifacts,'pocket-engineer-android-apk/android-signing-status.txt'),'utf8')).trim();
if(!signing.startsWith('project-release-key')&&!signing.startsWith('development-key;'))throw Error('Missing or unrecognized Android signing provenance');
const packages=[
  ['pocket-engineer-android-apk/android/app/build/outputs/apk/release/app-release.apk','PocketEngineer-0.5.0-android-'+(signing==='project-release-key'?'release':'preview')+'.apk'],
  ['pocket-engineer-windows-latest/PocketEngineer-0.5.0-win64.zip','PocketEngineer-0.5.0-Windows-x64.zip'],
  ['pocket-engineer-macos-latest/PocketEngineer-0.5.0-Darwin.zip','PocketEngineer-0.5.0-macOS-universal.zip'],
  ['pocket-engineer-ubuntu-latest/PocketEngineer-0.5.0-Linux.zip','PocketEngineer-0.5.0-Linux-x64.zip']
];
for(const [source] of packages)if(!(await stat(path.join(artifacts,source))).isFile())throw new Error('Missing package: '+source);
for(const source of ['index.html','engine.js','engine.wasm','service-worker.js'])if(!(await stat(path.join(website,source))).isFile())throw new Error('Incomplete offline website: '+source);
await mkdir(target);
for(const [source,dest] of packages)await copyFile(path.join(artifacts,source),path.join(target,dest));
execFileSync('zip',['-q','-r',path.join(target,'PocketEngineer-0.5.0-offline-website.zip'),'.'],{cwd:path.resolve(website)});
await writeFile(path.join(target,'BUILD_SOURCE.txt'),`Source commit: ${commit}\nVersion: 0.5.0-rc1\nBinaries copied unchanged from successful GitHub Actions artifacts.\nAndroid signing: ${signing}.\nAndroid download is an optimized, non-debuggable Release build.\nDesktop ZIPs contain Start-Pocket-Engineer launchers; Linux C/C++ runtime is statically linked.\nDesktop executables are not code-signed/notarized.\nWebsite ZIP requires localhost or HTTPS serving; file:// does not support this app. Use a desktop ZIP for a ready-to-launch local app.\nSee the release notes for platform instructions, CI runs and test limitations.\n`);
const sums=[];
for(const file of (await readdir(target)).sort())sums.push(`${createHash('sha256').update(await readFile(path.join(target,file))).digest('hex')}  ${file}`);
await writeFile(path.join(target,'SHA256SUMS.txt'),sums.join('\n')+'\n');
console.log(sums.join('\n'));
