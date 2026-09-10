import {readFile,stat,writeFile} from 'node:fs/promises';
import {createHash} from 'node:crypto';
const assets=['index.html','styles.css','app.js','offline.js','lab-ui.js','demos.js','demo-worker.js','solver-worker.js','engine.js','engine.wasm','manifest.webmanifest','assets/logo.webp','assets/icon-32.png','assets/icon-180.png','assets/icon-192.png','assets/icon-512.png','assets/linear-algebra.webp'];
const files=[];let total=0;
for(const asset of assets){const data=await readFile('www/'+asset);if(!data.length)throw new Error('Empty asset: '+asset);files.push({path:asset,bytes:data.length,sha256:createHash('sha256').update(data).digest('hex')});total+=(await stat('www/'+asset)).size;}
if(total>1500000)throw new Error('Critical offline bundle exceeds the 1.5 MB budget: '+total);
const demos=JSON.parse(await readFile('www/demo-data/manifest.json','utf8'));
if(demos.cases!==825000||demos.sets.length!==165)throw Error('Missing complete demo bank');
for(const bank of demos.sets){const data=await readFile('www/'+bank.file);if(data.length!==bank.bytes||createHash('sha256').update(data).digest('hex')!==bank.sha256)throw Error('Demo bank integrity mismatch: '+bank.file);files.push({path:bank.file,bytes:data.length,sha256:bank.sha256});}
const bankManifest=await readFile('www/demo-data/manifest.json');files.push({path:'demo-data/manifest.json',bytes:bankManifest.length,sha256:createHash('sha256').update(bankManifest).digest('hex')});
const allBytes=files.reduce((sum,file)=>sum+file.bytes,0);if(allBytes>12000000)throw Error('Complete offline bundle exceeds 12 MB, including every demo');
const manifest=JSON.stringify({files,core_bytes:total,total_bytes:allBytes})+'\n';await writeFile('www/offline-manifest.json',manifest);
const manifestHash=createHash('sha256').update(manifest).digest('hex'),version='pocket-engineer-'+manifestHash.slice(0,20);
const file='www/service-worker.js';
await writeFile(file,(await readFile(file,'utf8')).replace(/const VERSION='[^']+';/,`const VERSION='${version}';`).replace(/const MANIFEST_HASH='[^']+';/,`const MANIFEST_HASH='${manifestHash}';`));
console.log(JSON.stringify({version,critical_offline_bytes:total,budget_bytes:1500000,complete_offline_bytes:allBytes,demo_cases:demos.cases}));
