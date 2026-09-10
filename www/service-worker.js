'use strict';
const VERSION='pocket-engineer-89a19bded03fe6b5d497';
const MANIFEST_HASH='89a19bded03fe6b5d497408399431ba2b9fd691d0e9cbe3df51ed4e2f2fc2eb6';
const ROOT=new URL('./',self.location.href),MANIFEST=new URL('offline-manifest.json',ROOT).href;
let repairJob,manifestJob;
const digest=async response=>[...new Uint8Array(await crypto.subtle.digest('SHA-256',await response.clone().arrayBuffer()))].map(x=>x.toString(16).padStart(2,'0')).join('');
async function verified(url,hash){
  const response=await fetch(url,{cache:'reload',signal:AbortSignal.timeout(20000)});
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
      if(!cached||(checkExisting&&await digest(cached)!==file.sha256))await cache.put(url,await verified(url,file.sha256));
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
    const response=await verified(target,file?.sha256||MANIFEST_HASH);await cache.put(target,response.clone());return response;
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
