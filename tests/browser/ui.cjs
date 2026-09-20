// Exercise the same stage buttons and Advanced disclosures a person uses.
// Never force a click or change hidden/open attributes to bypass navigation.
async function reveal(target){
  await target.waitFor({state:'attached'});
  const stage=await target.evaluate(n=>n.closest('.flow-step[hidden]')?.id);
  if(stage)await target.page().locator(`.flow-steps button[aria-controls="${stage}"]`).click();
  const closed=target.locator('xpath=ancestor::details[not(@open)]');
  while(await closed.count())await closed.first().locator(':scope > summary').click();
}
function ui(target){return Object.fromEntries(['click','fill','selectOption','check','uncheck','press','focus'].map(method=>[method,async(...args)=>{
  // Output and editor can both offer PNG export; prefer the visible control.
  const visible=target.filter({visible:true});
  const control=await target.count()>1&&await visible.count()===1?visible:target;
  await reveal(control);return control[method](...args);
}]))}
module.exports={ui,reveal};
