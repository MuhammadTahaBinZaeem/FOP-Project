// Exercise every stored bank through the real downloaded UI and retain the
// export-button comparison records. Snapshot equality is not an independent oracle.
const {chromium}=require('playwright');
const {expect}=require('@playwright/test');
const fs=require('node:fs/promises');
const {createWriteStream}=require('node:fs');
const {createGzip}=require('node:zlib');
const {pipeline}=require('node:stream/promises');
(async()=>{
 const output=process.argv[2];if(!output||!process.env.PE_SITE_URL)throw Error('Provide a new output directory and explicit PE_SITE_URL');await fs.mkdir(output);await fs.mkdir(output+'/records');
 const browser=await chromium.launch({headless:true,executablePath:process.env.PE_BROWSER_PATH||undefined}),context=await browser.newContext({viewport:{width:412,height:844},acceptDownloads:true}),page=await context.newPage();
 const report={source:process.env.PE_SOURCE_COMMIT||'unspecified',site:process.env.PE_SITE_URL,started_at:new Date().toISOString(),kind:'stored_snapshot_comparison_not_independent_mathematical_proof',banks:[],page_errors:[]};page.on('pageerror',e=>report.page_errors.push(e.message));
 try{
  await page.goto(process.env.PE_SITE_URL);await expect(page.locator('body')).toHaveAttribute('data-engine','wasm',{timeout:60000});await expect(page.locator('body')).toHaveAttribute('data-offline-ready','true',{timeout:180000});
  const banks=await page.evaluate(async()=>{const m=await(await fetch('demo-data/manifest.json')).json();if(m.cases!==825000||m.sets.length!==165)throw Error('Incomplete bank');return m.sets;});await context.setOffline(true);
  await page.locator('.tool-launcher [data-view=demos]').click();await expect(page.locator('#demo-all')).toBeEnabled();
  for(const bank of banks){
   for(const [id,value] of [['demo-domain',bank.domain],['demo-topic',bank.topic],['demo-difficulty',bank.difficulty]])if(await page.locator('#'+id).inputValue()!==value){await page.locator('#'+id).selectOption(value);await expect(page.locator('#demo-all')).toBeEnabled({timeout:40000});}
   const before=Date.now();await page.locator('#demo-all').click();await expect(page.locator('#demo-status')).toContainText('Completed: 5000 cases; 0 differ',{timeout:180000});await expect(page.locator('#demo-export')).toBeEnabled();
   const event=page.waitForEvent('download');await page.locator('#demo-export').click();const download=await event;if(await download.failure())throw Error('Comparison export failed');
   const key=bank.domain+'.'+bank.topic+'.'+bank.difficulty;if(!/^[a-z0-9_.]+$/.test(key))throw Error('Invalid bank name');const stream=await download.createReadStream();await pipeline(stream,createGzip({level:9}),createWriteStream(output+'/records/'+key+'.json.gz'));
   const row={domain:bank.domain,topic:bank.topic,difficulty:bank.difficulty,cases:5000,mismatches:0,elapsed_including_export_ms:Date.now()-before,ui_summary:await page.locator('#demo-summary').textContent(),records:key+'.json.gz'};report.banks.push(row);await fs.writeFile(output+'/report.json',JSON.stringify(report,null,2));console.log(JSON.stringify({completed:report.banks.length,of:165,...row}));
  }
  expect(report.page_errors).toEqual([]);report.cases=report.banks.length*5000;report.finished_at=new Date().toISOString();report.success=true;await page.screenshot({path:output+'/last-bank.png'});
 }catch(error){report.success=false;report.error=error.message;report.demo_status=await page.locator('#demo-status').textContent().catch(()=>null);await page.screenshot({path:output+'/failure.png'}).catch(()=>{});throw error;
 }finally{await fs.writeFile(output+'/report.json',JSON.stringify(report,null,2));await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
