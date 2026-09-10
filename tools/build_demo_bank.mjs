import {readFile,writeFile,mkdir,readdir} from 'node:fs/promises';
import {gzipSync} from 'node:zlib';
import {createHash} from 'node:crypto';
// Repackage the ACTUAL checked regression corpus; never manufacture expected
// answers at runtime by asking the solver that is being tested.
const aliases={simplification:'simplify',linear_equations:'linear_equation',quadratic_equations:'quadratic_equation',kmap:'kmap_minimization',combinational:'combinational_logic',sequential:'sequential_logic',dc_nodal:'dc_nodal_analysis',mesh:'mesh_analysis',source_transform:'source_transformation'};
const digest=value=>createHash('sha256').update(value).digest('hex');
const root='test-data',destination='www/demo-data';await mkdir(destination,{recursive:true});const banks=[];let rows=0,bytes=0;
for(const domain of (await readdir(root,{withFileTypes:true})).filter(e=>e.isDirectory()).map(e=>e.name).sort()){
  for(const topic of (await readdir(`${root}/${domain}`,{withFileTypes:true})).filter(e=>e.isDirectory()).map(e=>e.name).sort()){
    for(const difficulty of ['easy','medium','hard']){
      const source=await readFile(`${root}/${domain}/${topic}/${difficulty}.jsonl`),records=source.toString().trim().split('\n').map(line=>JSON.parse(line));
      if(records.length!==5000)throw Error(`Expected 5000 records: ${domain}/${topic}/${difficulty}`);
      for(const [index,r] of records.entries())if(r.id!==`${domain}.${topic}.${difficulty}.${index}`||r.domain!==domain||r.topic!==topic||r.difficulty!==difficulty||typeof r.input!=='string'||typeof r.expected_answer!=='string'||typeof r.expected_verification!=='string')throw Error('Invalid corpus row: '+r.id);
      const compact=Buffer.from(JSON.stringify(records.map(r=>[r.input,r.expected_answer,r.expected_verification]))),compressed=gzipSync(compact,{level:9});
      const file=`${domain}.${topic}.${difficulty}.json.gz`;await writeFile(`${destination}/${file}`,compressed);rows+=records.length;bytes+=compressed.length;
      banks.push({domain,topic:aliases[topic]||topic,source_topic:topic,difficulty,count:records.length,file:`demo-data/${file}`,bytes:compressed.length,expanded_bytes:compact.length,sha256:digest(compressed),expanded_sha256:digest(compact),source_jsonl_sha256:digest(source)});
    }
  }
}
if(banks.length!==165||rows!==825000)throw Error('Incomplete 55-topic regression bank');
const manifest={version:1,source:'deterministic_regression_snapshot',description:'Original generated regression snapshots, not independent answer oracles. Compare actual answers and verification status; independent mathematical stress tests are documented separately.',banks:165,topics:55,cases:rows,compressed_bytes:bytes,sets:banks};
await writeFile(`${destination}/manifest.json`,JSON.stringify(manifest)+'\n');console.log(JSON.stringify({banks:banks.length,cases:rows,compressed_bytes:bytes}));
