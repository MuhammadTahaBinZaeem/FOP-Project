'use strict';
// Inflate and hold one selected bank OFF the UI thread. No solving logic here.
let rows=[],bank;
const sha=async bytes=>[...new Uint8Array(await crypto.subtle.digest('SHA-256',bytes))].map(b=>b.toString(16).padStart(2,'0')).join('');
self.onmessage=async({data})=>{
  const {id,method}=data;
  try{
    if(method==='load'){
      const next=data.bank;if(!/^demo-data\/[a-z0-9_]+\.[a-z0-9_]+\.(easy|medium|hard)\.(pebank|json\.gz)$/.test(next.file)||next.count!==5000||next.expanded_bytes>8000000)throw Error('Invalid demo manifest entry');
      if(typeof DecompressionStream==='undefined')throw Error('This browser/WebView needs an update to load compressed demos (DecompressionStream is unavailable). Core solving is still available.');
      // Android AssetLoader serves document requests but not this worker's
      // fetch. The WebView transfers compressed bytes; hashing/inflation stay here.
      let packed=data.packed;if(!(packed instanceof ArrayBuffer)){const response=await fetch(next.file,{signal:AbortSignal.timeout(30000)});if(!response.ok)throw Error('Demo set is not cached. Prepare offline access while connected, then retry.');packed=await response.arrayBuffer();}
      if(packed.byteLength!==next.bytes||await sha(packed)!==next.sha256)throw Error('Demo download integrity check failed; repair offline files.');
      const reader=new Blob([packed]).stream().pipeThrough(new DecompressionStream('gzip')).getReader(),chunks=[];let total=0;
      try{while(true){const {done,value}=await reader.read();if(done)break;total+=value.byteLength;if(total>next.expanded_bytes)throw Error('Decompressed demo data exceeds its declared size');chunks.push(value);}}finally{await reader.cancel().catch(()=>{});}
      const bytes=new Uint8Array(total);let offset=0;for(const chunk of chunks){bytes.set(chunk,offset);offset+=chunk.length;}
      if(total!==next.expanded_bytes||await sha(bytes)!==next.expanded_sha256)throw Error('Decompressed demo integrity check failed');
      const parsed=JSON.parse(new TextDecoder('utf-8',{fatal:true}).decode(bytes));if(!Array.isArray(parsed)||parsed.length!==5000||parsed.some(r=>!Array.isArray(r)||r.length!==3||r.some(v=>typeof v!=='string')||r[0].length>4096||r[1].length>12000))throw Error('Invalid demo records');
      rows=parsed;bank=next;self.postMessage({id,result:{count:rows.length}});
    }else if(method==='page'){
      if(!bank||!Number.isInteger(data.start)||data.start<0||data.start>=rows.length)throw Error('Choose a demo set first');
      self.postMessage({id,result:rows.slice(data.start,data.start+25).map((row,index)=>({index:data.start+index,input:row[0],expected_answer:row[1],expected_verification:row[2]}))});
    }else throw Error('Unknown demo operation');
  }catch(error){self.postMessage({id,error:error.message});}
};
