// Check GitHub's uploaded bytes before publishing a prepared draft release.
import {execFileSync} from 'node:child_process';
import {readFile,readdir,writeFile} from 'node:fs/promises';
import {createHash} from 'node:crypto';
import path from 'node:path';
import assert from 'node:assert/strict';
const [id,directory,output]=process.argv.slice(2);
if(!/^\d+$/.test(id||'')||!directory||!output)throw Error('Provide release ID, prepared files and a new report filename');
const release=JSON.parse(execFileSync('gh',['api',`repos/MuhammadTahaBinZaeem/FOP-Project/releases/${id}`],{encoding:'utf8'}));
const source=(await readFile(path.join(directory,'BUILD_SOURCE.txt'),'utf8')).match(/^Source commit: ([a-f0-9]{40})$/m)?.[1];
assert(source);assert.equal(release.target_commitish,source);
const files=(await readdir(directory)).sort();assert.equal(release.assets.length,files.length);
const checked=[];
for(const name of files){
  const asset=release.assets.find(a=>a.name===name);assert(asset,name);assert.equal(asset.state,'uploaded',name);
  const bytes=await readFile(path.join(directory,name)),sha256=createHash('sha256').update(bytes).digest('hex');
  assert.equal(asset.size,bytes.length,name);assert.equal(asset.digest,'sha256:'+sha256,name);checked.push({name,bytes:bytes.length,sha256});
}
const report={release_id:release.id,tag:release.tag_name,source,draft:release.draft,checked_at:new Date().toISOString(),assets:checked,success:true};
await writeFile(output,JSON.stringify(report,null,2)+'\n',{flag:'wx'});console.log(JSON.stringify(report));
