import {readFile,stat,writeFile} from 'node:fs/promises';
import {createHash} from 'node:crypto';
const assets=['index.html','styles.css','app.js','solver-worker.js','engine.js','engine.wasm','manifest.webmanifest','assets/logo.webp','assets/icon-32.png','assets/icon-180.png','assets/icon-192.png','assets/icon-512.png','assets/linear-algebra.webp'];
const hash=createHash('sha256');let total=0;
for(const asset of assets){const data=await readFile('www/'+asset);if(!data.length)throw new Error('Empty asset: '+asset);hash.update(asset);hash.update(data);total+=(await stat('www/'+asset)).size;}
if(total>1500000)throw new Error('Critical offline bundle exceeds the 1.5 MB budget: '+total);
const version='pocket-engineer-'+hash.digest('hex').slice(0,20);
const file='www/service-worker.js';
await writeFile(file,(await readFile(file,'utf8')).replace(/const VERSION='[^']+';/,`const VERSION='${version}';`));
console.log(JSON.stringify({version,critical_offline_bytes:total,budget_bytes:1500000}));
