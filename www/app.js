const $ = (id) => document.getElementById(id);
const form = $('solve-form'), result = $('result');
const nativeBridge = () => window.PocketEngineerAndroid && typeof window.PocketEngineerAndroid.solve === 'function';
async function identifyOffline(input) {
  if (nativeBridge()) return JSON.parse(window.PocketEngineerAndroid.identify(input));
  const response = await fetch('/api/identify', {method:'POST', headers:{'Content-Type':'application/json'}, body: JSON.stringify({input})});
  return response.json();
}
async function solveOffline(request) {
  if (nativeBridge()) return JSON.parse(window.PocketEngineerAndroid.solve(JSON.stringify(request)));
  const response = await fetch('/api/solve', {method:'POST', headers:{'Content-Type':'application/json'}, body: JSON.stringify(request)});
  return response.json();
}
function display(data) {
  result.classList.remove('empty');
  if (data.status !== 'success') { result.innerHTML = `<h3>Needs attention</h3><p>${data.answer.text}</p>`; return; }
  const verified = data.verification.status.replace('_', ' ');
  const steps = data.steps.map(s => `<li><strong>${s.explanation}</strong><code>${s.expression}</code></li>`).join('');
  result.innerHTML = `<span class="badge">${verified} · ${data.verification.method}</span><h3>Answer</h3><p class="answer">${data.answer.text}</p>${data.assumptions.length ? `<p><strong>Assumption:</strong> ${data.assumptions.join('; ')}</p>` : ''}<ol class="steps">${steps}</ol><p><small>${data.verification.evidence}</small></p>`;
}
async function solve(event) {
  if (event) event.preventDefault();
  let request = {domain: $('domain').value, topic: $('topic').value, input: $('input').value};
  result.className = 'result empty'; result.textContent = 'Identifying the problem type locally…';
  try {
    const identification = await identifyOffline(request.input), candidate = identification.candidates[0];
    if (candidate) {
      const accepted = window.confirm(`Pocket Engineer identified: ${candidate.domain.replaceAll('_', ' ')} → ${candidate.topic.replaceAll('_', ' ')}.\n\nConfirm this before solving?`);
      if (!accepted) { result.textContent = 'Choose the correct subject and topic, then solve again.'; return; }
      request = {...request, domain: candidate.domain, topic: candidate.topic}; $('domain').value = candidate.domain; $('topic').value = candidate.topic;
    }
    result.textContent = 'Solving locally and checking the result…';
    display(await solveOffline(request));
  }
  catch { result.innerHTML = '<h3>Offline engine unavailable</h3><p>Desktop: run <code>pocket-engineer-server</code> and open <code>http://127.0.0.1:8080</code>. Android: install the package with the native Pocket Engineer bridge.</p>'; }
}
form.addEventListener('submit', solve);
document.querySelectorAll('[data-domain]').forEach(button => button.addEventListener('click', () => { $('domain').value=button.dataset.domain; $('topic').value=button.dataset.topic; $('input').value=button.dataset.input; solve(); }));
let installPrompt; window.addEventListener('beforeinstallprompt', event => { event.preventDefault(); installPrompt=event; $('install').hidden=false; }); $('install').addEventListener('click', async () => { if(installPrompt){installPrompt.prompt(); await installPrompt.userChoice; $('install').hidden=true;} });
if ('serviceWorker' in navigator && !nativeBridge()) window.addEventListener('load', () => navigator.serviceWorker.register('/service-worker.js').catch(() => {}));
