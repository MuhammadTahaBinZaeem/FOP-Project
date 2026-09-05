'use strict';
// A separate worker keeps the Android/desktop browser UI responsive. Responses
// own native heap strings only until JSON.parse completes; always release them.
let modulePromise;
function engine() {
  if(!modulePromise)modulePromise=Promise.resolve().then(()=>{
    importScripts('engine.js');
    return createPocketEngineer({locateFile:name=>new URL(name,self.location.href).href});
  });
  return modulePromise;
}
self.onmessage=async({data})=>{
  const {id,method,payload}=data;
  try {
    const names={solve:'pe_solve_json',identify:'pe_identify_json',catalog:'pe_catalog_json'};
    if(!Object.prototype.hasOwnProperty.call(names,method))throw new Error('Unsupported engine request');
    const m=await engine(),hasInput=method!=='catalog';
    const pointer=m.ccall(names[method],'number',hasInput?['string']:[],hasInput?[payload]:[]);
    if(!pointer)throw new Error('Engine could not allocate a response');
    try {
      // ccall's string conversion exposes a non-owning UTF-8 view without a
      // second native request; UTF8ToString is exported for this purpose.
      const result=JSON.parse(m.UTF8ToString(pointer));self.postMessage({id,result});
    }finally{m.ccall('pe_free_string',null,['number'],[pointer]);}
  }catch(error){self.postMessage({id,error:error.message||String(error)});}
};
