'use strict';
// Browser delivery only. Calculations and data validation stay in C++.
window.PEOffline=(()=>{
  const get=id=>document.getElementById(id);
  let mode='',job,prompt;
  function display(message,ready=false){get('cache-status').textContent=message;get('offline-status').textContent=message;document.body.dataset.offlineReady=String(ready);}
  function message(worker,type){return new Promise((resolve,reject)=>{
    const channel=new MessageChannel(),timer=setTimeout(()=>{channel.port1.close();reject(Error('Offline download timed out. Partial files are retained; check your connection and retry.'));},type==='REPAIR_OFFLINE'?180000:15000);
    channel.port1.onmessage=event=>{clearTimeout(timer);channel.port1.close();resolve(event.data);};
    worker.postMessage({type},[channel.port2]);
  });}
  function activated(worker){return new Promise((resolve,reject)=>{
    if(worker.state==='activated'){resolve(worker);return;}
    const timer=setTimeout(()=>finish(Error('Offline installation is taking longer than expected. Partial files are retained; retry when connected.')),180000);
    function finish(error){clearTimeout(timer);worker.removeEventListener('statechange',changed);error?reject(error):resolve(worker);}
    function changed(){if(worker.state==='activated')finish();else if(worker.state==='redundant')finish(Error('Offline installation failed. Reconnect and retry to repair it.'));}
    worker.addEventListener('statechange',changed);changed();
  });}
  async function run(repair=false){
    if(mode==='android'){display('Ready offline · C++ engine and interface bundled in the APK.',true);return;}
    if(mode==='http'){display('Native server mode: keep the downloaded desktop launcher running. For server-free use, install the Render website.');return;}
    if(!isSecureContext||!('serviceWorker' in navigator))throw Error('Offline installation requires HTTPS or localhost and service-worker support.');
    let registration=await navigator.serviceWorker.getRegistration();
    if((repair&&navigator.onLine)||!registration||(!registration.active&&!registration.installing&&!registration.waiting))registration=await navigator.serviceWorker.register('service-worker.js?refresh='+Date.now(),{scope:'./',updateViaCache:'none'});
    get('update-offline').hidden=!registration.waiting;
    registration.addEventListener('updatefound',()=>{const worker=registration.installing;worker?.addEventListener('statechange',()=>{get('update-offline').hidden=!registration.waiting;});});
    let worker=registration.active;
    if(!worker){
      const installing=registration.installing||registration.waiting;
      if(!installing)throw Error('No offline worker is available. Retry to start the installation.');
      display('Downloading the app and all 825,000 demos (about 10 MB)… Keep this tab open.');worker=await activated(installing);
    }
    let result=await message(worker,'CHECK_OFFLINE');
    if(repair){
      display(`Repairing offline files: ${result.completed}/${result.total} cached…`);
      const progress=setInterval(()=>{message(worker,'CHECK_OFFLINE').then(r=>display(`Downloading offline files: ${r.completed}/${r.total} cached…`)).catch(()=>{});},1500);
      try{result=await message(worker,'REPAIR_OFFLINE');}finally{clearInterval(progress);}
    }
    if(result.error)throw Error(result.error);
    display(result.ready?`Ready offline · solver and all 825,000 demos saved (${result.total} files). Revisit ${location.origin}${new URL('./',location.href).pathname} in this browser without internet.`:`Offline files incomplete (${result.completed}/${result.total}). Press Prepare / repair offline access while connected.`,result.ready);
    if(result.ready&&mode==='failed'&&repair)window.dispatchEvent(new Event('pe-retry-engine'));
    return result;
  }
  function check(repair=false){
    if(job)return job;
    const button=get('retry-cache');button.disabled=true;button.textContent=repair?'Preparing offline access…':'Checking offline files…';
    display(repair?'Preparing offline access…':'Checking offline files…');
    job=run(repair).catch(error=>display(error.message)).finally(()=>{button.disabled=false;button.textContent='Prepare / repair offline access';job=null;});
    return job;
  }
  get('retry-cache').addEventListener('click',()=>check(true));
  get('update-offline').addEventListener('click',async()=>{
    const registration=await navigator.serviceWorker.getRegistration();
    if(!registration?.waiting){get('update-offline').hidden=true;return;}
    window.dispatchEvent(new Event('pe-save-draft'));
    navigator.serviceWorker.addEventListener('controllerchange',()=>location.reload(),{once:true});
    registration.waiting.postMessage({type:'ACTIVATE'});
  });
  get('protect-offline').addEventListener('click',async()=>{
    try{const granted=await navigator.storage?.persist?.();get('storage-status').textContent=granted?'Persistent storage granted. Clearing browser/site data still removes offline files.':'This browser did not grant persistent storage. Offline access works while its site data is retained; install the app and avoid clearing site data.';}
    catch{get('storage-status').textContent='Storage protection is unavailable. Keep site data to retain offline access.';}
  });
  window.addEventListener('beforeinstallprompt',event=>{event.preventDefault();prompt=event;get('install').hidden=false;});
  get('install').addEventListener('click',async()=>{if(!prompt)return;try{await prompt.prompt();await prompt.userChoice;}catch{display('Installation was not completed. Retry using your browser install menu.');}finally{prompt=null;get('install').hidden=true;}});
  window.addEventListener('appinstalled',()=>{get('install').hidden=true;});
  return {check,setMode(value){mode=value;return check(false);}};
})();
