// Stage verified CI binaries for Render. This does not publish source code,
// rebuild executables, or change GitHub repository visibility.
import {access,copyFile,mkdir,readFile,readdir,writeFile} from 'node:fs/promises';
import {execFileSync} from 'node:child_process';
import {createHash} from 'node:crypto';
import path from 'node:path';
const [prepared,tag,notes]=process.argv.slice(2);
if(!prepared||!/^v0\.5\.0-rc\d+$/.test(tag||'')||!notes)throw Error('Provide prepared release, tag and release notes');
const target=path.resolve('distribution',tag);
const provenance=await readFile(path.join(prepared,'BUILD_SOURCE.txt'),'utf8');
if(!provenance.includes('Version: '+tag.slice(1)+'\n')||!/^Source commit: [a-f0-9]{40}$/m.test(provenance))throw Error('Prepared build provenance does not match the public version');
try{await access(target);throw Error('Refusing to overwrite a staged release');}catch(e){if(e.code!=='ENOENT')throw e;}
const sums=new Map((await readFile(path.join(prepared,'SHA256SUMS.txt'),'utf8')).trim().split('\n').map(line=>{
  const m=line.match(/^([a-f0-9]{64})  ([A-Za-z0-9_.-]+)$/);if(!m)throw Error('Invalid prepared checksum');return [m[2],m[1]];
}));
const files=await readdir(prepared);if(files.length!==sums.size+1)throw Error('Unchecked prepared file');
for(const [name,expected] of sums){if(createHash('sha256').update(await readFile(path.join(prepared,name))).digest('hex')!==expected)throw Error('Prepared bytes changed: '+name);}
await mkdir(target,{recursive:true});for(const name of sums.keys())await copyFile(path.join(prepared,name),path.join(target,name));
await copyFile(notes,path.join(target,'RELEASE_NOTES.txt'));
execFileSync('zip',['-q','-r',path.join(target,'PocketEngineer-0.5.0-test-evidence.zip'),'docs'],{cwd:process.cwd()});
const records=[];for(const name of (await readdir(target)).sort())records.push(createHash('sha256').update(await readFile(path.join(target,name))).digest('hex')+'  '+name);
await writeFile(path.join(target,'SHA256SUMS.txt'),records.join('\n')+'\n');
console.log(JSON.stringify({tag,target,checksummed_files:records.length}));
