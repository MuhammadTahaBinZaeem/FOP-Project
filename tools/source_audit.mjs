import {readdir,stat,writeFile} from 'node:fs/promises';
import path from 'node:path';
// Audit only the maintained product, not the archived original Qt application,
// generated corpora, Emscripten runtime, third-party libraries or build outputs.
const roots=['src','include','apps','android/app/src/main','www'];
const cpp=new Set(['.cpp','.hpp','.h','.cc','.cxx']);
const code=new Set([...cpp,'.js','.mjs','.cjs','.kt','.java','.ts','.tsx','.jsx']);
const presentation=new Set(['.html','.css']);
const totals={cpp:0,other_runtime_code:0,html_css:0};
const files=[];
async function walk(dir){
  for(const entry of await readdir(dir,{withFileTypes:true})){
    const file=path.join(dir,entry.name);
    if(entry.isDirectory()){await walk(file);continue;}
    if(['engine.js','engine.wasm'].includes(entry.name))continue;
    const ext=path.extname(file);
    if(!code.has(ext)&&!presentation.has(ext))continue;
    const bytes=(await stat(file)).size;
    const kind=cpp.has(ext)?'cpp':presentation.has(ext)?'html_css':'other_runtime_code';
    totals[kind]+=bytes;files.push({file,bytes,kind});
  }
}
for(const root of roots)await walk(root);
const runtime=totals.cpp+totals.other_runtime_code;
const result={scope:'Maintained production runtime source bytes; excludes tests, generators, archives, vendor and generated files',...totals,cpp_percent_runtime:100*totals.cpp/runtime,cpp_percent_including_html_css:100*totals.cpp/(runtime+totals.html_css),files};
console.log(JSON.stringify(result,null,2));
if(process.argv[2])await writeFile(process.argv[2],JSON.stringify(result,null,2)+'\n');
// HTML and CSS are separately reported so the denominator is never hidden.
if(result.cpp_percent_runtime<80)process.exitCode=1;
