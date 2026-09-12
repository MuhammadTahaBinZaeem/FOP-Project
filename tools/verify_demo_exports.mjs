// Audit exported UI records against the actual bank bytes, not only UI counters.
import {readFile,writeFile} from 'node:fs/promises';
import {gunzipSync} from 'node:zlib';
import {createHash} from 'node:crypto';
import assert from 'node:assert/strict';
import path from 'node:path';
const [directory,website,output]=process.argv.slice(2);
if(!directory||!website||!output)throw Error('Provide exported run, website and new report path');
const report=JSON.parse(await readFile(path.join(directory,'report.json')));
const manifest=JSON.parse(await readFile(path.join(website,'demo-data/manifest.json')));
assert.equal(report.success,true);assert.equal(report.banks.length,165);assert.equal(manifest.sets.length,165);
let checked=0;
for(const bank of manifest.sets){
  const key=`${bank.domain}.${bank.topic}.${bank.difficulty}`;
  assert.match(key,/^[a-z0-9_.]+$/);
  const packed=await readFile(path.join(website,bank.file));assert.equal(createHash('sha256').update(packed).digest('hex'),bank.sha256);
  const original=JSON.parse(gunzipSync(packed));assert.equal(original.length,5000);
  const saved=JSON.parse(gunzipSync(await readFile(path.join(directory,'records',key+'.json.gz'))));
  assert.equal(saved.bank.file,bank.file);assert.equal(saved.bank.sha256,bank.sha256);assert.equal(saved.tested,5000);assert.equal(saved.mismatches,0);assert.equal(saved.results.length,5000);
  const indices=new Set();
  for(const row of saved.results){
    assert(Number.isInteger(row.index)&&row.index>=0&&row.index<5000);assert(!indices.has(row.index));indices.add(row.index);
    const expected=original[row.index];assert.equal(row.input,expected[0]);assert.equal(row.expected_answer,expected[1]);assert.equal(row.expected_verification,expected[2]);
    assert.equal(row.actual_answer,expected[1]);assert.equal(row.actual_verification,expected[2]);assert.equal(row.actual_status,'success');assert.equal(row.matches,true);
    assert(Number.isFinite(row.elapsed_ms)&&row.elapsed_ms>=0);checked++;
  }
}
const result={source:report.source,checked_at:new Date().toISOString(),banks:165,cases:checked,success:true,comparison:'Every exported input, expected answer, actual answer, verification label and unique index matches its hashed source bank. Snapshot equality is not independent mathematical proof.'};
await writeFile(output,JSON.stringify(result,null,2)+'\n',{flag:'wx'});console.log(JSON.stringify(result));
