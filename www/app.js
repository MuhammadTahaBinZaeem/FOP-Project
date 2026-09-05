/* Presentation only. Identification, solving, limits and catalog live in C++. */
'use strict';
const $ = id => document.getElementById(id);
const subjects = {algebra:'Algebra',calculus:'Calculus',linear_algebra:'Linear algebra',differential_equations:'Differential equations',logic:'Digital logic · DLD',circuit:'Circuits · LCA / ENA',engineering:'Engineering units',programming:'C++ fundamentals'};
const state = {catalog:[],mode:'',worker:null,sequence:0,pending:new Map(),result:null,problem:null,busy:false,history:[]};
const storageKey = 'pocket-engineer.history.v3';
function node(tag,text,className) { const n=document.createElement(tag);if(text!==undefined)n.textContent=text;if(className)n.className=className;return n; }
function tell(message) { $('notice').textContent=message; }
function view(id) {
  if(!['workbench','subjects','history','downloads'].includes(id))return;
  document.querySelectorAll('.view').forEach(n=>n.hidden=n.id!==id);
  document.querySelectorAll('.nav').forEach(n=>{const active=n.dataset.view===id;n.classList.toggle('active',active);if(active)n.setAttribute('aria-current','page');else n.removeAttribute('aria-current');});
  if(id==='history')renderHistory();
  document.body.dataset.view=id;
  window.scrollTo(0,0);
}
document.querySelectorAll('[data-view]').forEach(n=>n.addEventListener('click',event=>{event.preventDefault();view(n.dataset.view);}));
window.peHandleBack=()=>{if(document.body.dataset.view&&document.body.dataset.view!=='workbench'){view('workbench');return true;}return false;};
function currentTopic() { return state.catalog.find(t=>t.domain===$('domain').value&&t.topic===$('topic').value); }
function updateTopics(preferred) {
  const list=state.catalog.filter(t=>t.domain===$('domain').value);
  $('topic').replaceChildren(...list.map(t=>new Option(t.title,t.topic)));
  if(list.some(t=>t.topic===preferred))$('topic').value=preferred;
  updateHint();
}
function updateHint() { const t=currentTopic();if(!t)return;$('syntax').textContent=t.syntax;$('scope').textContent=t.scope;$('input').placeholder=t.example; }
function selectTopic(t,example=true) { if(state.busy)return; $('domain').value=t.domain;updateTopics(t.topic);if(example)$('input').value=t.example;view('workbench');$('input').focus(); }
$('domain').addEventListener('change',()=>updateTopics());
$('topic').addEventListener('change',updateHint);
$('example').addEventListener('click',()=>{const t=currentTopic();if(t){$('input').value=t.example;tell('Example loaded. You can change the values before solving.');$('input').focus();}});
function renderSubjects() {
  const query=$('search').value.trim().toLowerCase();$('subject-list').replaceChildren();
  for(const [domain,title] of Object.entries(subjects)) {
    const topics=state.catalog.filter(t=>t.domain===domain&&[title,t.title,t.scope,t.topic].join(' ').toLowerCase().includes(query));
    if(!topics.length)continue;const card=node('article',undefined,'subject-card');card.append(node('h2',title));
    topics.forEach(t=>{const b=node('button',t.title+' →','topic-link');b.addEventListener('click',()=>selectTopic(t));card.append(b);});$('subject-list').append(card);
  }
  if(!$('subject-list').children.length)$('subject-list').append(node('p','No matching topics. Try a subject name.','hint'));
}
$('search').addEventListener('input',renderSubjects);
function loadHistory() {
  try { const rows=JSON.parse(localStorage.getItem(storageKey)||'[]');if(Array.isArray(rows))state.history=rows.filter(r=>r&&typeof r.input==='string'&&r.input.length<=4096&&typeof r.domain==='string'&&typeof r.topic==='string'&&typeof r.at==='number').slice(0,30); } catch {state.history=[];}
}
function saveHistory(problem) {
  state.history=[{...problem,at:Date.now()},...state.history.filter(r=>r.domain!==problem.domain||r.topic!==problem.topic||r.input!==problem.input)].slice(0,30);
  try {localStorage.setItem(storageKey,JSON.stringify(state.history));}catch{tell('Solved. Device storage is unavailable; this problem could not be saved.');}
}
function renderHistory() {
  $('history-list').replaceChildren();
  if(!state.history.length)$('history-list').append(node('p','A fresh page. Your next solved problem will appear here.','hint'));
  state.history.forEach(r=>{const b=node('button',undefined,'history-item');b.append(node('strong',r.input),node('small',(subjects[r.domain]||r.domain)+' · '+new Date(r.at).toLocaleString()));b.addEventListener('click',()=>{const t=state.catalog.find(t=>t.domain===r.domain&&t.topic===r.topic);if(t){selectTopic(t,false);$('input').value=r.input;tell('Previous input restored. Solve again to recompute it with the current engine.');}});$('history-list').append(b);});
}
$('clear-history').addEventListener('click',()=>{state.history=[];try{localStorage.removeItem(storageKey);}catch{}renderHistory();});
function settle(id,result,error) {
  const job=state.pending.get(id);if(!job)return;clearTimeout(job.timer);state.pending.delete(id);
  if(error)job.reject(new Error(error));else job.resolve(result);
}
window.peNativeResult=(id,json)=>{try{settle(id,JSON.parse(json));}catch{settle(id,null,'Invalid native response');}};
function workerCall(method,payload) {
  return new Promise((resolve,reject)=>{
    const id=++state.sequence;
    const timer=setTimeout(()=>{settle(id,null,'The solver timed out. Reload the page to restart it, then use a smaller input.');if(state.worker){state.worker.terminate();state.worker=null;}state.mode='failed';$('solve').disabled=true;$('identify').disabled=true;$('engine-status').textContent='Engine stopped';},method==='catalog'?120000:15000);
    state.pending.set(id,{resolve,reject,timer});
    if(state.mode==='android')window.PocketEngineerAndroid.request(id,method,payload||'');
    else state.worker.postMessage({id,method,payload});
  });
}
async function request(method,payload) {
  if(state.mode==='http') {
    const response=await fetch('api/'+method,{signal:AbortSignal.timeout(15000),method:method==='catalog'?'GET':'POST',headers:{'Content-Type':'application/json'},body:method==='catalog'?undefined:method==='identify'?JSON.stringify({input:payload}):payload});
    if(!response.ok)throw new Error('Local server returned '+response.status);
    return response.json();
  }
  return workerCall(method,payload);
}
async function boot() {
  try {
    let catalog;
    if(window.PocketEngineerAndroid){state.mode='android';catalog=await request('catalog');}
    else {
      state.mode='wasm';
      try {
        if(!window.WebAssembly||!window.Worker)throw new Error('WebAssembly workers are not supported');
        state.worker=new Worker('solver-worker.js');
        state.worker.onmessage=e=>settle(e.data.id,e.data.result,e.data.error);
        state.worker.onerror=()=>{for(const id of [...state.pending.keys()])settle(id,null,'Browser engine could not load');};
        catalog=await request('catalog');
      } catch(error) {
        if(state.worker)state.worker.terminate();state.worker=null;
        state.mode='http';catalog=await request('catalog');
      }
    }
    if(!Array.isArray(catalog.topics)||!catalog.topics.length)throw new Error('Topic catalog unavailable');
    state.catalog=catalog.topics;
    $('domain').replaceChildren(...Object.entries(subjects).map(([value,label])=>new Option(label,value)));
    updateTopics();$('input').value=currentTopic().example;
    ['domain','topic','solve','identify','example'].forEach(id=>$(id).disabled=false);
    document.body.dataset.engine=state.mode;
    $('engine-status').textContent=state.mode==='wasm'?'C++ · on your device':state.mode==='android'?'Native C++ · offline':'Native C++ · local server';
    $('engine-status').classList.add('ready');renderSubjects();loadHistory();setupOffline();
  } catch(error) {
    $('engine-status').textContent='Engine unavailable';
    tell('Could not start the local engine. Connect once to download the website, or use a native package. Reload to retry. '+error.message);
    $('cache-status').textContent='Solver not loaded; offline solving is not ready.';
  }
}
function setBusy(value){state.busy=value;['solve','identify','domain','topic','example'].forEach(id=>$(id).disabled=value);$('input').readOnly=value;$('solve-form').setAttribute('aria-busy',String(value));}
$('solve-form').addEventListener('submit',async event=>{
  event.preventDefault();if(state.busy)return;
  const problem={domain:$('domain').value,topic:$('topic').value,input:$('input').value.trim()};
  if(!problem.input){tell('Enter a problem or load an example first.');$('input').focus();return;}
  setBusy(true);tell('Working locally…');
  $('input').blur();
  try {
    const result=await request('solve',JSON.stringify(problem));state.problem=problem;state.result=result;
    renderResult(result);tell(result.status==='error'?'Check your input against the example and supported syntax.':'');
    if(result.status==='success')saveHistory(problem);
    $('result').focus({preventScroll:true});$('result').scrollIntoView({block:'start'});
  } catch(error){tell(error.message);}
  finally {setBusy(false);if(state.mode==='failed'){$('solve').disabled=true;$('identify').disabled=true;}}
});
$('input').addEventListener('keydown',event=>{if(event.key==='Enter'&&(event.ctrlKey||event.metaKey)){event.preventDefault();$('solve-form').requestSubmit();}});
$('identify').addEventListener('click',async()=>{
  if(state.busy||!$('input').value.trim())return;setBusy(true);
  try {
    const r=await request('identify',$('input').value);
    const c=r.candidates&&r.candidates[0], t=c&&state.catalog.find(t=>t.domain===c.domain&&t.topic===c.topic);
    if(t){$('domain').value=t.domain;updateTopics(t.topic);tell('Suggested: '+t.title+'. Confirm the selected type and input syntax, then press “Show me the steps.” '+r.reason);}
    else tell((r.reason||'No precise type found.')+' Choose a subject and problem type manually.');
  }catch(e){tell(e.message);}finally{setBusy(false);}
});
function renderResult(r) {
  $('result').hidden=false;$('copy').textContent='Copy solution';$('result-topic').textContent=state.catalog.find(t=>t.domain===state.problem?.domain&&t.topic===state.problem?.topic)?.title||r.topic;
  $('answer').textContent=r.answer?.text||'No answer returned.';
  const v=r.verification||{},ok=r.status==='success'&&v.status!=='not_verified'&&v.status!=='verification_failed';
  $('verification').className='verification'+(ok?'':' warning');
  $('verification').replaceChildren(node('strong',r.status==='error'?'Input could not be solved':ok?'Method check · '+(v.method||'reported by engine'):'Review required · '+(v.method||'not verified')),node('span',v.evidence||'No verification evidence available.'));
  $('steps').replaceChildren(...(r.steps||[]).map(s=>{const li=node('li');li.append(node('p',s.explanation),node('pre',s.expression));return li;}));
  $('caveats').replaceChildren(...[...(r.assumptions||[]),...(r.warnings||[])].map(t=>node('p',t)));
  renderVisual(r.visual);
}
function renderVisual(raw) {
  $('visual').replaceChildren();let v;try{v=JSON.parse(raw||'{}');}catch{return;}
  if(v.kind==='kmap'&&Array.isArray(v.cells)){
    const rowBits=Math.floor(v.variables/2),colBits=v.variables-rowBits,table=node('table',undefined,'kmap');
    table.append(node('caption','Gray-code map · row variables first, column variables last. X = don’t care.'));
    const header=node('tr');header.append(node('th','↓ / →'));
    for(let c=0;c<2**colBits;c++)header.append(node('th',(c^(c>>1)).toString(2).padStart(colBits,'0')));table.append(header);
    for(let r=0;r<2**rowBits;r++){const row=node('tr'),gray=r^(r>>1);row.append(node('th',gray.toString(2).padStart(rowBits,'0')));for(let c=0;c<2**colBits;c++){const value=v.cells[(gray<<colBits)|(c^(c>>1))];row.append(node('td',value,value==='1'?'on':''));}table.append(row);}
    $('visual').append(table);
  }
  if(v.kind==='trajectory'&&Array.isArray(v.points)&&v.points.length>1){
    const canvas=node('canvas');canvas.width=900;canvas.height=280;canvas.setAttribute('role','img');canvas.setAttribute('aria-label',v.label||'Numerical solution trajectory; exact endpoint and error are listed in the steps.');$('visual').append(canvas);
    const ctx=canvas.getContext('2d'),points=v.points.filter(p=>p.length===2&&p.every(Number.isFinite));if(!ctx||points.length<2)return;
    const xs=points.map(p=>p[0]),ys=points.map(p=>p[1]),xmin=Math.min(...xs),xmax=Math.max(...xs),ymin=Math.min(...ys),ymax=Math.max(...ys),dx=xmax-xmin||1,dy=ymax-ymin||1;
    ctx.strokeStyle='#cbd5c0';ctx.lineWidth=1;ctx.beginPath();ctx.moveTo(65,20);ctx.lineTo(65,240);ctx.lineTo(870,240);ctx.stroke();
    ctx.fillStyle='#536553';ctx.font='13px monospace';ctx.fillText(ymax.toPrecision(4),4,26);ctx.fillText(ymin.toPrecision(4),4,237);ctx.fillText(xmin.toPrecision(3),65,264);ctx.fillText(xmax.toPrecision(3),803,264);ctx.fillText('x',882,243);
    ctx.strokeStyle='#1b4538';ctx.lineWidth=3;ctx.beginPath();points.forEach(([x,y],i)=>{const px=65+800*(x-xmin)/dx,py=235-205*(y-ymin)/dy;i?ctx.lineTo(px,py):ctx.moveTo(px,py);});ctx.stroke();
  }
}
function solutionText() {
 const r=state.result;return [state.problem.input,'',r.answer?.text||'','',...(r.steps||[]).map((s,i)=>(i+1)+'. '+s.explanation+'\n'+s.expression),'',r.verification?.method||'',r.verification?.evidence||'',...(r.assumptions||[]),...(r.warnings||[])].join('\n');
}
$('copy').addEventListener('click',async()=>{try{if(state.mode==='android')window.PocketEngineerAndroid.copySolution(solutionText());else await navigator.clipboard.writeText(solutionText());$('copy').textContent='Copied';}catch{$('copy').textContent='Select the result to copy';}});
$('export').addEventListener('click',()=>{if(!state.result)return;if(state.mode==='android'){window.PocketEngineerAndroid.saveSolution(JSON.stringify({problem:state.problem,result:state.result},null,2));return;}const url=URL.createObjectURL(new Blob([JSON.stringify({problem:state.problem,result:state.result},null,2)],{type:'application/json'})),a=node('a');a.href=url;a.download='pocket-engineer-solution.json';a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);});
$('print').addEventListener('click',()=>state.mode==='android'?window.PocketEngineerAndroid.printSolution():window.print());
let installPrompt;
window.addEventListener('beforeinstallprompt',event=>{event.preventDefault();installPrompt=event;$('install').hidden=false;});
$('install').addEventListener('click',async()=>{if(installPrompt){try{await installPrompt.prompt();await installPrompt.userChoice;}catch{$('cache-status').textContent='Installation was not completed. Use your browser’s install menu to retry.';}finally{installPrompt=null;$('install').hidden=true;}}});
window.addEventListener('appinstalled',()=>{$('install').hidden=true;});
async function checkCache(){
  if(state.mode==='android'){$('cache-status').textContent='Ready offline · native engine and interface bundled in the APK.';return;}
  if(state.mode==='http'){$('cache-status').textContent='Native server mode: keep the local desktop server running. Install the published website for server-free offline use.';return;}
  try {
    const registration=await navigator.serviceWorker.getRegistration();
    const worker=registration?.active;
    if(!worker)throw new Error('Offline installation is still in progress. Check again in a moment.');
    const channel=new MessageChannel();
    const ready=await new Promise((resolve,reject)=>{const timer=setTimeout(()=>reject(new Error('Offline check timed out. Retry.')),8000);channel.port1.onmessage=e=>{clearTimeout(timer);channel.port1.close();resolve(e.data.ready);};worker.postMessage({type:'CHECK_OFFLINE'},[channel.port2]);});
    $('cache-status').textContent=ready?'Ready offline · interface and C++ solver are cached on this device.':'Offline files are incomplete. Reconnect and reload, then check again.';
    $('offline-status').textContent=ready?'Ready offline. Your next solution needs no connection.':'Keep this page online until the solver finishes caching.';
    document.body.dataset.offlineReady=String(ready);
  }catch(error){$('cache-status').textContent=error.message;}
}
async function setupOffline(){
  if(state.mode==='android'||state.mode==='http'){checkCache();return;}
  if(!('serviceWorker' in navigator)){$('cache-status').textContent='Offline caching is unavailable in this browser.';return;}
  try {await navigator.serviceWorker.register('service-worker.js',{scope:'./'});await navigator.serviceWorker.ready;await checkCache();}
  catch{$('cache-status').textContent='Could not cache the offline app. Check storage space, reconnect and reload.';}
}
$('retry-cache').addEventListener('click',checkCache);
boot();
