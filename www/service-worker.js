'use strict';
const VERSION='pocket-engineer-83c5e74604ce6a2f0097';
const MANIFEST_HASH='83c5e74604ce6a2f0097d767b79ab3e34d1a196290e2db2e2a07f7bf0a760562';
const ROOT=new URL('./',self.location.href),MANIFEST=new URL('offline-manifest.json',ROOT).href;
let repairJob,manifestJob,imagesJob;
const digest=async response=>[...new Uint8Array(await crypto.subtle.digest('SHA-256',await response.clone().arrayBuffer()))].map(x=>x.toString(16).padStart(2,'0')).join('');
async function verified(url,hash){
  const source=new URL(url);source.searchParams.set('pe',hash);
  const response=await fetch(source,{cache:'reload',signal:AbortSignal.timeout(20000)});
  if(!response.ok)throw Error('Could not download '+new URL(url).pathname+' ('+response.status+')');
  if(await digest(response)!==hash)throw Error('Website changed during download. Check for an update, then retry.');
  return response;
}
async function manifest(){
  if(!manifestJob)manifestJob=(async()=>{
    const cache=await caches.open(VERSION);let response=await cache.match(MANIFEST);
    if(!response||await digest(response)!==MANIFEST_HASH){response=await verified(MANIFEST,MANIFEST_HASH);await cache.put(MANIFEST,response.clone());}
    const data=await response.json();if(!Array.isArray(data.files)||data.files.length>300)throw Error('Invalid offline manifest');return data;
  })().catch(error=>{manifestJob=null;throw error;});return manifestJob;
}
async function verifiedFile(file,data){
  if(!file.archive)return verified(new URL(file.path,ROOT).href,file.sha256);
  if(!imagesJob)imagesJob=(async()=>{const archive=data.files.find(f=>f.path===file.archive);if(!archive)throw Error('Missing raster archive');const cache=await caches.open(VERSION),url=new URL(archive.path,ROOT).href;let r=await cache.match(url);if(!r||await digest(r)!==archive.sha256){r=await verified(url,archive.sha256);await cache.put(url,r.clone());}return r.json();})().catch(e=>{imagesJob=null;throw e;});
  const encoded=(await imagesJob)[file.path];if(typeof encoded!=='string'||encoded.length>300000)throw Error('Invalid raster archive');const bytes=Uint8Array.from(atob(encoded),c=>c.charCodeAt(0)),r=new Response(bytes,{headers:{'Content-Type':file.mime}});if(bytes.length!==file.bytes||await digest(r)!==file.sha256)throw Error('Raster integrity check failed');return r;
}
async function status(){
  const data=await manifest(),cache=await caches.open(VERSION),missing=[];
  if(!await cache.match(MANIFEST)){manifestJob=null;await manifest();}
  for(const file of data.files)if(!await cache.match(new URL(file.path,ROOT).href))missing.push(file.path);
  return {ready:missing.length===0,completed:data.files.length-missing.length,total:data.files.length,missing,version:VERSION,bytes:data.total_bytes};
}
async function repair(checkExisting=false){
  if(repairJob)return repairJob;
  repairJob=(async()=>{
    const data=await manifest(),cache=await caches.open(VERSION);
    for(let i=0;i<data.files.length;i+=3)await Promise.all(data.files.slice(i,i+3).map(async file=>{
      const url=new URL(file.path,ROOT).href,cached=await cache.match(url);
      if(!cached||(checkExisting&&await digest(cached)!==file.sha256))await cache.put(url,await verifiedFile(file,data));
    }));return status();
  })().finally(()=>{repairJob=null;});return repairJob;
}
self.addEventListener('install',event=>event.waitUntil(repair()));
self.addEventListener('activate',event=>event.waitUntil((async()=>{
  for(const key of await caches.keys())if(key.startsWith('pocket-engineer-')&&key!==VERSION)await caches.delete(key);
  await self.clients.claim();
})()));
self.addEventListener('fetch',event=>{
  const url=new URL(event.request.url);
  if(event.request.method!=='GET'||url.origin!==ROOT.origin||!url.pathname.startsWith(ROOT.pathname)||url.pathname.includes('/api/'))return;
  const navigation=event.request.mode==='navigate';url.search='';url.hash='';
  const target=navigation?new URL('index.html',ROOT).href:url.href;
  // Register respondWith synchronously; load the immutable file list within it
  // so worker restarts do not depend on retained global state.
  event.respondWith((async()=>{
    const data=await manifest(),file=data.files.find(f=>new URL(f.path,ROOT).href===target);
    if(!file&&target!==MANIFEST)return fetch(event.request);
    const cache=await caches.open(VERSION),cached=await cache.match(target);if(cached)return cached;
    const response=file?await verifiedFile(file,data):await verified(target,MANIFEST_HASH);await cache.put(target,response.clone());return response;
  })().catch(()=>new Response('Offline files are missing. Reconnect and use Prepare / repair offline access.',{status:503,headers:{'Content-Type':'text/plain'}})));
});
self.addEventListener('message',event=>{
  if(event.data?.type==='ACTIVATE'){event.waitUntil(self.skipWaiting());return;}
  if(!['CHECK_OFFLINE','REPAIR_OFFLINE'].includes(event.data?.type))return;
  event.waitUntil((async()=>{
    try{event.ports[0]?.postMessage(event.data.type==='REPAIR_OFFLINE'?await repair(true):await status());}
    catch(error){event.ports[0]?.postMessage({ready:false,error:error.message});}
  })());
});
