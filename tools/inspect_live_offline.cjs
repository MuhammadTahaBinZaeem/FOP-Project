// Read cache/registration state and compare deployed bytes without deleting data.
const {chromium}=require('playwright');
const fs=require('node:fs/promises');
(async()=>{
 const [profile,output]=process.argv.slice(2);if(!profile||!output)throw Error('Provide an existing test profile and a new report filename');
 const context=await chromium.launchPersistentContext(profile,{headless:true,executablePath:process.env.PE_BROWSER_PATH||undefined}),page=context.pages()[0]||await context.newPage();
 try{await page.goto('https://pocket-engineer.onrender.com/');await page.waitForFunction(()=>document.body.dataset.engine==='wasm');await page.evaluate(()=>PEOffline.check(true));
 const report=await page.evaluate(async()=>{
  const hash=async r=>Array.from(new Uint8Array(await crypto.subtle.digest('SHA-256',await r.arrayBuffer())),b=>b.toString(16).padStart(2,'0')).join('');
  const data={status:document.querySelector('#cache-status').textContent,registrations:[],caches:[]};
  for(const r of await navigator.serviceWorker.getRegistrations())data.registrations.push({scope:r.scope,active:r.active?.state,installing:r.installing?.state,waiting:r.waiting?.state});
  for(const key of await caches.keys()){const cache=await caches.open(key),keys=await cache.keys(),r=await cache.match(new URL('offline-manifest.json',location).href);if(!r)continue;const m=await r.json(),missing=[],bad=[];
   for(const f of m.files){const stored=await cache.match(new URL(f.path,location).href);if(!stored)missing.push(f);else if(await hash(stored)!==f.sha256)bad.push(f.path);}
   const probes=[];for(const f of missing.slice(0,4)){const response=await fetch(f.path,{cache:'reload'});probes.push({path:f.path,status:response.status,expected:f.sha256,actual:await hash(response.clone()),headers:Object.fromEntries(response.headers)});}
   data.caches.push({key,count:keys.length,missing:missing.map(f=>f.path),bad,probes});
  }return data;
 });await fs.writeFile(output,JSON.stringify(report,null,2));console.log(JSON.stringify(report,null,2));
 }finally{await context.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
