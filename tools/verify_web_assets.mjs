import {readFile,stat,writeFile} from 'node:fs/promises';
import {createHash} from 'node:crypto';
const assets=['index.html','styles.css','app.js','offline.js','lab-ui.js','demos.js','demo-worker.js','solver-worker.js','engine.js','engine.wasm','manifest.webmanifest','assets/logo.webp','assets/icon-32.png','assets/icon-180.png','assets/icon-192.png','assets/icon-512.png','assets/linear-algebra.webp'];
// Version the whole executable chain, before hashing its parents. Versioning
// only HTML scripts allowed a current UI to start an old cached solver worker.
// Keep literal URLs for the offline cache, subdirectory hosting and APK loader.
const keyFor=async name=>createHash('sha256').update(await readFile('www/'+name)).digest('hex').slice(0,16);
async function bind(file,dependency){
  const source=await readFile('www/'+file,'utf8');
  const pattern=new RegExp("'"+dependency.replaceAll('.','\\.')+"(?:\\?pe=[a-f0-9]+)?'",'g');
  if([...source.matchAll(pattern)].length!==1)throw Error('Expected one engine dependency: '+file+' -> '+dependency);
  await writeFile('www/'+file,source.replace(pattern,"'"+dependency+'?pe='+await keyFor(dependency)+"'"));
}
await bind('solver-worker.js','engine.js');
await bind('solver-worker.js','engine.wasm');
await bind('app.js','solver-worker.js');
// New entry HTML must not pick up a previous generation of its bootstrap code.
// Hash query keys also work with the canonical offline cache and Android loader.
let index=await readFile('www/index.html','utf8');
for(const name of ['styles.css','offline.js','app.js','lab-ui.js','demos.js']){
  const key=createHash('sha256').update(await readFile('www/'+name)).digest('hex').slice(0,16);
  index=index.replace(new RegExp('((?:src|href)="'+name.replaceAll('.','\\.')+')(?:\\?pe=[a-f0-9]+)?"'),`$1?pe=${key}"`);
}
await writeFile('www/index.html',index);
const files=[],rasters={};let total=0;
for(const asset of assets){const data=await readFile('www/'+asset);if(!data.length)throw new Error('Empty asset: '+asset);const raster=/\.(png|webp)$/.test(asset);if(raster)rasters[asset]=data.toString('base64');files.push({path:asset,bytes:data.length,sha256:createHash('sha256').update(data).digest('hex'),...(raster?{archive:'offline-images.json',mime:asset.endsWith('.png')?'image/png':'image/webp'}:{})});total+=(await stat('www/'+asset)).size;}
// Render/Cloudflare Polish can transform PNG bytes after deployment. Keep a
// verified JSON transport of the original rasters; never relax asset hashes.
const rasterData=JSON.stringify(rasters)+'\n';await writeFile('www/offline-images.json',rasterData);files.push({path:'offline-images.json',bytes:Buffer.byteLength(rasterData),sha256:createHash('sha256').update(rasterData).digest('hex')});total+=Buffer.byteLength(rasterData);
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
