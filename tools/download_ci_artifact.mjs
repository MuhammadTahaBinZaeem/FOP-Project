// Download one known CI artifact in bounded parallel ranges, verify GitHub's
// SHA-256, then extract into a NEW directory. Never log credentials or signed URLs.
import {execFileSync} from 'node:child_process';
import {mkdir,writeFile} from 'node:fs/promises';
import {createHash} from 'node:crypto';
import path from 'node:path';
const [id,output]=process.argv.slice(2);
if(!/^\d+$/.test(id||'')||!output)throw Error('Provide an artifact ID and a new output directory');
const repo='MuhammadTahaBinZaeem/FOP-Project',endpoint=`repos/${repo}/actions/artifacts/${id}`;
const metadata=JSON.parse(execFileSync('gh',['api',endpoint],{encoding:'utf8'}));
if(metadata.expired||metadata.size_in_bytes>150000000||!/^sha256:[a-f0-9]{64}$/.test(metadata.digest||''))throw Error('Artifact expired, too large or missing digest');
await mkdir(output);
const token=execFileSync('gh',['auth','token'],{encoding:'utf8'}).trim();
const response=await fetch(`https://api.github.com/${endpoint}/zip`,{redirect:'manual',signal:AbortSignal.timeout(30000),headers:{Authorization:`Bearer ${token}`,Accept:'application/vnd.github+json'}});
if(response.status!==302)throw Error('Artifact authorization failed: '+response.status);
const url=new URL(response.headers.get('location'));
if(url.protocol!=='https:'||!['.blob.core.windows.net','.githubusercontent.com'].some(s=>url.hostname.endsWith(s)))throw Error('Unexpected artifact storage host');
const size=metadata.size_in_bytes,chunkSize=1024*1024,chunks=new Array(Math.ceil(size/chunkSize));let cursor=0,completed=0;
await Promise.all(Array.from({length:6},async()=>{
  while(cursor<chunks.length){
    const index=cursor++,start=index*chunkSize,end=Math.min(size-1,start+chunkSize-1);let data;
    for(let attempt=0;attempt<3;attempt++){
      try{
        const part=await fetch(url,{headers:{Range:`bytes=${start}-${end}`},signal:AbortSignal.timeout(120000)});
        if(part.status!==206||part.headers.get('content-range')!==`bytes ${start}-${end}/${size}`)throw Error('Invalid range response');
        data=Buffer.from(await part.arrayBuffer());if(data.length!==end-start+1)throw Error('Truncated range');break;
      }catch{if(attempt===2)throw Error('Artifact range failed after three attempts: '+index);}
    }
    chunks[index]=data;completed+=data.length;console.log(JSON.stringify({artifact:id,downloaded_bytes:completed,total_bytes:size}));
  }
}));
const zip=Buffer.concat(chunks),digest='sha256:'+createHash('sha256').update(zip).digest('hex');
if(digest!==metadata.digest)throw Error('Artifact SHA-256 mismatch');
const archive=path.resolve(output,'artifact.zip');await writeFile(archive,zip);
const entries=execFileSync('unzip',['-Z1',archive],{encoding:'utf8'}).trim().split('\n');
if(entries.some(n=>n.startsWith('/')||n.includes('\\')||n.split('/').includes('..')))throw Error('Unsafe archive entry');
const destination=path.resolve(output,'contents');await mkdir(destination);execFileSync('unzip',['-q',archive,'-d',destination]);
// Retain the verified archive, metadata and extraction provenance for the audit.
await writeFile(path.join(output,'download.json'),JSON.stringify({id,name:metadata.name,source:metadata.workflow_run?.head_sha,bytes:zip.length,digest,verified:true,downloaded_at:new Date().toISOString()},null,2)+'\n');
console.log(JSON.stringify({artifact:id,destination,digest_verified:true}));
