#!/usr/bin/env python3
"""Generate a local, self-contained review overlay; never connects to hardware.

Capture PNGs with firmware_console first. The classifier below mirrors the current local
main.cpp::touch; this is a source-derived target map, not touch-frequency data.
"""
import argparse
import base64
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
COLORS = {'setup':'#38bdf8','link':'#f87171','outdoor':'#fb923c',
          'wind':'#a78bfa','humidity':'#4ade80','rain':'#22d3ee',
          'pressure':'#facc15','alerts':'#f87171','cycle':'#a78bfa',
          'toggle':'#4ade80','next':'#fb923c','back':'#ec4899',
          'recovery':'#facc15','none':'#94a3b8'}
LABELS = {'setup':'Open settings','link':'Open Outdoor / status',
          'outdoor':'Open Outdoor / status','wind':'Open Wind details',
          'humidity':'Open Humidity details','rain':'Open Rain details',
          'pressure':'Open Pressure details','alerts':'Open NWS alerts',
          'cycle':'Cycle trend measurement','toggle':'Toggle daily / hourly',
          'next':'Next forecast period','back':'Close / Back',
          'recovery':'Open Wi-Fi recovery details','none':'No tap action'}
DETAILS = {'outdoor','wind','humidity','rain','pressure','link'}
PAGES = ['home','trends','forecast','outdoor','wind','humidity','rain','pressure','settings','alerts']


def action(screen, x, y, wifi_left, wifi_right):
    if screen == 'settings':
        return 'back' if y < 44 else 'recovery' if 193 <= y < 228 else 'none'
    if screen == 'alerts':
        return 'back' if y < 40 else 'next_alert' if y > 205 else 'none'
    if screen in DETAILS:
        return 'back' if y < 44 else 'alerts' if y >= 188 else 'none'
    if y < 44:
        return 'link' if x < 104 else 'setup'
    if y >= 188:
        return 'alerts'
    if screen == 'forecast':
        return 'toggle' if y < 62 else 'next'
    if screen == 'trends':
        return 'cycle' if y < 187 else 'none'
    if y < 184:
        return ('outdoor' if x < 180 else 'wind') if y < 136 else (
            'humidity' if x < 108 else 'rain' if x < 212 else 'pressure')
    return 'none'


def rectangles(screen, left, right):
    """Merge identical horizontal pixel runs into exclusive target rectangles."""
    finished, active = [], {}
    for y in range(240):
        runs = []
        start, key = 0, action(screen, 0, y, left, right)
        for x in range(1, 321):
            other = action(screen, x, y, left, right) if x < 320 else None
            if other != key:
                runs.append((start, x, key)); start, key = x, other
        next_active = {}
        for run in runs:
            rect = active.pop(run, [run[0], y, run[1]-run[0], 0, run[2]])
            rect[3] += 1
            next_active[run] = rect
        finished.extend(active.values()); active = next_active
    return finished + list(active.values())


def schematic(screen):
    rows = ([('WEB SETTINGS',20),('Change settings in a web browser',60),('Local URL / IP',100),('Admin guidance',150),
             ('Wi-Fi recovery / details',210)] if screen == 'settings' else
            [('NWS ALERTS',20),('Alert headline and text',80),('Drag to scroll',140),
             ('Tap bottom: next alert',224)])
    body = ''.join(f'<text x="12" y="{y}" fill="#d8e6fa" font-size="13">{s}</text>' for s,y in rows)
    svg = f'<svg xmlns="http://www.w3.org/2000/svg" width="320" height="240"><rect width="320" height="240" fill="#0b1523"/>{body}</svg>'
    return 'data:image/svg+xml;base64,' + base64.b64encode(svg.encode()).decode()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--captures', type=Path, default=ROOT/'captures/firmware-validation/touch-map-v075')
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    output = args.output or args.captures/'index.html'
    images = {}
    for page in PAGES:
        images[page] = schematic(page) if page in ('settings','alerts') else (
            'data:image/png;base64,' + base64.b64encode((args.captures/(page+'.png')).read_bytes()).decode())
    source = (ROOT/'firmware/src/main.cpp').read_text()
    touch = source.split('void touch() {',1)[1].split('\nvoid liveReceive()',1)[0]
    # Exact glyph-advance-derived bounds, from home_fonts::status and statusLayout.
    layouts = {'APRS':(240,278),'CWOP':(236,274)}
    data = {'images':images,'colors':dict(COLORS,next_alert='#fb923c'),
            'labels':dict(LABELS,next_alert='Next NWS alert'),
            'maps':{mode:{p:rectangles(p,*bounds) for p in PAGES} for mode,bounds in layouts.items()},
            'source_sha256':hashlib.sha256(touch.encode()).hexdigest()}
    html = '''<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>WX Link · touch target review</title><style>
:root{color-scheme:dark;font-family:system-ui,sans-serif;background:#08111e;color:#e8effa}body{max-width:1120px;margin:24px auto;padding:0 18px}h1{font-size:25px;margin-bottom:8px}p{color:#b2c0d4;line-height:1.5}.controls{display:flex;flex-wrap:wrap;gap:12px;align-items:center;background:#142238;padding:14px;border-radius:12px}select,button{font:inherit;padding:8px;background:#20334e;color:inherit;border:1px solid #48617d;border-radius:6px}button[aria-pressed=true]{background:#256081}.layout{display:grid;grid-template-columns:minmax(0,704px) minmax(230px,1fr);gap:20px;margin-top:20px}svg{width:100%;height:auto;display:block;background:#000;image-rendering:pixelated;border-radius:6px}svg rect.zone{pointer-events:none;stroke-width:.5}#readout{min-height:60px;background:#142238;padding:14px;border-radius:8px;color:#e8effa}#legend{display:grid;gap:8px}.key{display:flex;align-items:center;gap:10px}.swatch{height:15px;width:15px;flex:none;border-radius:3px}.note{font-size:13px}code{color:#e0b5fa}label{display:flex;align-items:center;gap:6px}#screen-title{text-transform:capitalize}details{margin-top:20px;background:#142238;padding:14px;border-radius:8px}li{margin:8px 0;color:#b2c0d4}@media(max-width:820px){.layout{grid-template-columns:1fr}}@media print{.controls{display:none}}
</style><h1>WX Link · touch target review</h1><p>Baseline v0.7.5 screen captures with v0.8.0 candidate touch zones. This is a target map, not a heatmap of recorded touches. Click or hover over the display to inspect a location; this page never controls the device.</p>
<div class="controls"><label>Screen <select id="screen"></select></label><label>Sharing label <select id="mode"><option>APRS</option><option>CWOP</option></select></label><button id="taps" aria-pressed="true">Tap zones</button><button id="swipes" aria-pressed="false">Swipe / drag zones</button><label>Overlay <input id="opacity" type="range" min="0" max="75" value="28"></label><label><input type="checkbox" id="wake">Dim/off: first touch</label></div>
<div class="layout"><div><h2 id="screen-title"></h2><svg id="display" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 320 240" role="img" aria-label="Weather display with touch target overlay"><image id="background" width="320" height="240"/><g id="zones"></g></svg><p id="caption" class="note"></p><p id="readout" role="status">Hover or click the display to see its exact action.</p></div><aside><h2>Active targets</h2><div id="legend"></div><p id="gesture" class="note"></p></aside></div>
<details open><summary>Boundaries worth reviewing</summary><ul><li>The full 44-pixel header uses two targets: x 0–103 opens Outdoor/status (red); x 104–319 opens web settings (blue), including callsign, clock and all link indicators.</li><li>Home: Outdoor/Wind split at x 180, y 44–135. Tiles use y 136–183, with splits at x 108 and 212; these extend slightly beyond the drawn cards.</li><li>Home y 184–187 has no tap action. Alerts starts at y 188, including the margin around the visible alert box. Trends y 187 is also inactive.</li><li>Page swipes must START at y 44–186. Horizontal travel must exceed 45 px and be more than 1.5× vertical travel. A normal primary-page tap must move less than 15 px on each axis and last under 1 second.</li><li>Detail-body swipes open trends only for Outdoor, Wind and Humidity. Rain, Pressure and Link details have no body-swipe navigation.</li><li>Forecast and Alerts handle vertical drags first, including drags starting in their headers. Forecast toggles daily/hourly only at y 44–61; taps lower down advance a period.</li><li>First touch while dimmed/off only wakes; its release is consumed. It should not open a tile or navigate. The separate overnight unresponsive-touch report remains unresolved.</li></ul></details>
<p class="note">Settings and Alerts are labeled schematics: no setup credentials or untriggered alert screenshots were captured. Other pages are local real screenshots. Detail maps assume the originating page is Home; when opened from Forecast, its vertical-scroll branch takes precedence. Factory-reset confirmation/hold gestures are outside this normal-navigation map. Current handler fingerprint: <code id="fingerprint"></code>.</p>
<script>const DATA=__DATA__;
const $=id=>document.getElementById(id), NS='http://www.w3.org/2000/svg';let gestureMode=false;
for(const name of Object.keys(DATA.images)){const o=document.createElement('option');o.value=name;o.textContent=name[0].toUpperCase()+name.slice(1)+(name==='settings'||name==='alerts'?' (schematic)':'');$('screen').append(o)}
function rect(x,y,w,h,key){const r=document.createElementNS(NS,'rect');for(const [k,v] of Object.entries({x,y,width:w,height:h,fill:DATA.colors[key]||'#38bdf8','fill-opacity':Number($('opacity').value)/100,stroke:DATA.colors[key]||'#38bdf8'}))r.setAttribute(k,v);r.classList.add('zone');$('zones').append(r)}
function refresh(){const p=$('screen').value,mode=$('mode').value;$('background').setAttribute('href',DATA.images[p]);$('screen-title').textContent=p;$('caption').textContent=p==='settings'||p==='alerts'?'Schematic only — exact handler geometry, illustrative background.':'Baseline v0.7.5 framebuffer; v0.8.0 candidate routing; backgrounds predate the combined Outdoor/Settings changes.';$('zones').replaceChildren();$('legend').replaceChildren();let keys=[];
if($('wake').checked){rect(0,0,320,240,'back');keys=['wake']}
else if(gestureMode){const horizontal=['home','trends','forecast','outdoor','wind','humidity'].includes(p);if(horizontal)rect(0,44,320,143,'link');if(['forecast','alerts'].includes(p))rect(0,0,320,240,'wind');keys=horizontal?['horizontal']:[];if(['forecast','alerts'].includes(p))keys.push('vertical')}
else{const rs=DATA.maps[mode][p];for(const [x,y,w,h,key] of rs){if(key!=='none')rect(x,y,w,h,key);if(!keys.includes(key))keys.push(key)}}
for(const k of keys){const row=document.createElement('div');row.className='key';const s=document.createElement('span');s.className='swatch';s.style.background=DATA.colors[k]||'#38bdf8';const t=document.createElement('span');t.textContent=DATA.labels[k]||({wake:'First contact wakes only',horizontal:'Horizontal swipe lane',vertical:'Vertical drag / scroll'})[k];row.append(s,t);$('legend').append(row)}
$('gesture').textContent=p==='settings'?'Recovery target appears only after setup and outside recovery mode; otherwise that region is inactive.':p==='alerts'?'Vertical movement >20 px and greater than horizontal movement scrolls; tap y 206–239 advances the alert.':'Swipe left: Home → Trends → Forecast → Home. Swipe right reverses the order. Details only support the documented measurement trend gesture.';
$('taps').setAttribute('aria-pressed',!gestureMode);$('swipes').setAttribute('aria-pressed',gestureMode);$('readout').textContent='Hover or click the display to inspect an action.'}
function inspect(e){const r=$('display').getBoundingClientRect(),x=Math.max(0,Math.min(319,Math.floor((e.clientX-r.left)*320/r.width))),y=Math.max(0,Math.min(239,Math.floor((e.clientY-r.top)*240/r.height)));const p=$('screen').value;const hit=DATA.maps[$('mode').value][p].find(([a,b,w,h])=>x>=a&&x<a+w&&y>=b&&y<b+h);$('readout').textContent=`x ${x}, y ${y} · `+($('wake').checked?'Wake only — release does not activate this target.':gestureMode?'Gesture origin: '+(y>=44&&y<187&&['home','trends','forecast','outdoor','wind','humidity'].includes(p)?'horizontal swipe allowed; ':'horizontal page swipe not enabled; ')+(['forecast','alerts'].includes(p)?'vertical scroll allowed.':'vertical scroll not enabled.'):DATA.labels[hit[4]])}
for(const id of ['screen','mode','opacity','wake'])$(id).addEventListener('input',refresh);$('taps').onclick=()=>{gestureMode=false;refresh()};$('swipes').onclick=()=>{gestureMode=true;refresh()};$('display').addEventListener('pointermove',inspect);$('display').addEventListener('click',inspect);$('fingerprint').textContent=DATA.source_sha256;refresh();
</script></html>'''
    output.parent.mkdir(parents=True,exist_ok=True)
    output.write_text(html.replace('__DATA__',json.dumps(data,separators=(',',':'))))
    print(output)


if __name__ == '__main__':
    main()
