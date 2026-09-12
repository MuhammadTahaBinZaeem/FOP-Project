// Public binary delivery from the existing Render site; the source repo stays
// private. Generated installers are never part of the app's offline manifest.
import {readdir,readFile,mkdir,copyFile} from 'node:fs/promises';
import {createHash} from 'node:crypto';
import path from 'node:path';
const source=process.argv[2]||'distribution',target=process.argv[3]||'www/downloads';
let releases;try{releases=await readdir(source,{withFileTypes:true});}catch(e){if(e.code==='ENOENT'){console.log('No public release mirror staged');process.exit(0);}throw e;}
for(const release of releases){
  if(!release.isDirectory())continue;
  if(!/^v\d+\.\d+\.\d+-rc\d+$/.test(release.name))throw Error('Invalid public release directory');
  const origin=path.join(source,release.name),destination=path.join(target,release.name);
  const sums=await readFile(path.join(origin,'SHA256SUMS.txt'),'utf8'),files=new Map();
  for(const line of sums.trim().split('\n')){
    const entry=line.match(/^([a-f0-9]{64})  ([A-Za-z0-9_.-]+)$/);if(!entry||files.has(entry[2]))throw Error('Invalid or duplicate checksum record');files.set(entry[2],entry[1]);
  }
  const names=await readdir(origin);if(names.length!==files.size+1||files.has('SHA256SUMS.txt'))throw Error('Mirror contains an unchecked file');
  for(const [name,expected] of files){const data=await readFile(path.join(origin,name));if(createHash('sha256').update(data).digest('hex')!==expected)throw Error('Mirror integrity mismatch: '+name);}
  await mkdir(destination,{recursive:true});for(const name of names)await copyFile(path.join(origin,name),path.join(destination,name));
  console.log(JSON.stringify({public_release:release.name,verified_files:files.size,publish_directory:destination}));
}
