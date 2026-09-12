// Inspect packaged APK bytes, not just the source asset directory. Android's
// asset merger can transform .gz files into differently named raw assets.
import {execFileSync} from 'node:child_process';
import {createHash} from 'node:crypto';
import {gunzipSync} from 'node:zlib';
const apk=process.argv[2];if(!apk)throw Error('Provide an assembled APK');
if(execFileSync('unzip',['-Z1',apk],{encoding:'utf8'}).split('\n').some(n=>n.startsWith('assets/downloads/')))throw Error('Public installers must not be bundled inside the APK');
const read=name=>execFileSync('unzip',['-p',apk,'assets/'+name],{maxBuffer:10000000});
const manifest=JSON.parse(read('demo-data/manifest.json'));let count=0;
for(const bank of manifest.sets){const bytes=read(bank.file);if(bytes.length!==bank.bytes||createHash('sha256').update(bytes).digest('hex')!==bank.sha256)throw Error('Packaged demo bytes changed: '+bank.file);const expanded=gunzipSync(bytes,{maxOutputLength:8000000});if(expanded.length!==bank.expanded_bytes||createHash('sha256').update(expanded).digest('hex')!==bank.expanded_sha256)throw Error('Expanded packaged demo mismatch');const rows=JSON.parse(expanded);if(rows.length!==5000)throw Error('Incomplete APK bank');count+=rows.length;}
if(count!==825000)throw Error('Missing packaged cases');console.log(JSON.stringify({apk,sets:manifest.sets.length,cases:count,compressed_and_expanded_hashes_verified:true}));
