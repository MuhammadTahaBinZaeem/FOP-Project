// Test an extracted download, not executables in the compiler's build folder.
// Usage: node tools/test_desktop_package.mjs archive.zip new-directory [SHA256SUMS.txt]
import {mkdir,readdir,readFile,writeFile,access} from 'node:fs/promises';
import {spawn,execFileSync} from 'node:child_process';
import {createHash} from 'node:crypto';
import path from 'node:path';
const [archiveArg,directoryArg,sumsArg]=process.argv.slice(2);
if(!archiveArg||!directoryArg)throw Error('Provide a ZIP and a new extraction directory');
const archive=path.resolve(archiveArg),directory=path.resolve(directoryArg);
const digest=createHash('sha256').update(await readFile(archive)).digest('hex');
if(sumsArg){const expected=(await readFile(sumsArg,'utf8')).split('\n').find(line=>line.endsWith('  '+path.basename(archive)));if(expected!==digest+'  '+path.basename(archive))throw Error('Published SHA256 mismatch');}
await mkdir(directory);execFileSync('cmake',['-E','tar','xf',archive],{cwd:directory});
async function find(root,name){const found=[];for(const entry of await readdir(root,{withFileTypes:true})){const p=path.join(root,entry.name);if(entry.isDirectory())found.push(...await find(p,name));else if(entry.name===name)found.push(p);}return found;}
const suffix=process.platform==='win32'?'.exe':'';
const servers=await find(directory,'pocket-engineer-server'+suffix);if(servers.length!==1)throw Error('Package must contain exactly one local server');
const server=servers[0],cli=path.join(path.dirname(server),'pocket-engineer'+suffix);
const question='Please find the determinant of [[1,2],[3,4]]';
const result=JSON.parse(execFileSync(cli,['auto',question],{cwd:directory,encoding:'utf8',timeout:15000}));
if(result.status!=='success'||result.answer.text!=='det(A) = -2')throw Error('Downloaded CLI answer mismatch');
const packageRoot=path.dirname(path.dirname(server));
await access(path.join(packageRoot,'START_HERE.txt'));
await access(path.join(packageRoot,'Start-Pocket-Engineer'+(process.platform==='win32'?'.cmd':process.platform==='darwin'?'.command':'.sh')));
const child=spawn(server,['0'],{cwd:directory,stdio:['ignore','pipe','pipe']});
let stdout='',stderr='';child.stderr.on('data',b=>stderr+=b);
const url=await new Promise((resolve,reject)=>{const timer=setTimeout(()=>{child.kill();reject(Error('Server startup timeout: '+stderr));},15000);child.on('error',e=>{clearTimeout(timer);reject(e);});child.on('exit',code=>{clearTimeout(timer);reject(Error('Server exited '+code+': '+stderr));});child.stdout.on('data',b=>{stdout+=b;const m=stdout.match(/http:\/\/127\.0\.0\.1:\d+/);if(m){clearTimeout(timer);resolve(m[0]);}});});
try{
  const html=await fetch(url);if(!html.ok||!(await html.text()).includes('auto-type'))throw Error('Downloaded website missing natural-input UI');
  const solved=await fetch(url+'/api/solve',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({domain:'logic',topic:'truth_table',mode:'auto',input:question})});
  const output=await solved.json();if(output.status!=='success'||output.answer.text!=='det(A) = -2'||!output.interpretation.type_changed)throw Error('Downloaded HTTP solver mismatch');
  const rejected=await fetch(url+'/api/solve',{method:'POST',headers:{'Content-Type':'application/json','Origin':'https://example.invalid'},body:'{}'});if(rejected.status!==403)throw Error('Cross-origin access was not blocked');
  const report={archive:path.basename(archive),sha256:digest,published_checksum_verified:!!sumsArg,platform:process.platform,architecture:process.arch,cli:true,relocatable_server:true,bundled_ui:true,natural_input:true,wrong_type_correction:true,cross_origin_rejected:true,localhost_url:url};
  await writeFile(path.join(directory,'download-test.json'),JSON.stringify(report,null,2)+'\n');console.log(JSON.stringify(report,null,2));
}finally{child.kill();}
