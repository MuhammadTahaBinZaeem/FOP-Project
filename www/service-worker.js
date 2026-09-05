'use strict';
const VERSION='pocket-engineer-v3-20260906';
const ROOT=new URL('./',self.location.href);
const FILES=['./','index.html','styles.css','app.js','solver-worker.js','engine.js','engine.wasm','manifest.webmanifest','assets/algebra.png','assets/linear-algebra.png'];
const URLS=FILES.map(file=>new URL(file,ROOT).href);
self.addEventListener('install',event=>{
  event.waitUntil((async()=>{const cache=await caches.open(VERSION);try{await cache.addAll(URLS);}catch(error){await caches.delete(VERSION);throw error;}})());
});
self.addEventListener('activate',event=>{
  event.waitUntil((async()=>{
    for(const name of await caches.keys())if(name.startsWith('pocket-engineer-')&&name!==VERSION)await caches.delete(name);
    await self.clients.claim();
  })());
});
self.addEventListener('fetch',event=>{
  const url=new URL(event.request.url);
  if(event.request.method!=='GET'||url.origin!==ROOT.origin||!url.pathname.startsWith(ROOT.pathname))return;
  if(url.pathname.includes('/api/'))return; // never cache solver requests
  if(!URLS.includes(url.href)&&event.request.mode!=='navigate')return;
  event.respondWith((async()=>{
    const cache=await caches.open(VERSION);
    const match=await cache.match(event.request);
    if(match)return match;
    try{return await fetch(event.request);}
    catch(error){if(event.request.mode==='navigate')return (await cache.match(new URL('index.html',ROOT).href))||Response.error();throw error;}
  })());
});
self.addEventListener('message',event=>{
  if(event.data?.type==='CHECK_OFFLINE')event.waitUntil((async()=>{
    const cache=await caches.open(VERSION);
    const ready=(await Promise.all(URLS.map(url=>cache.match(url)))).every(Boolean);
    event.ports[0]?.postMessage({ready,version:VERSION});
  })());
});
