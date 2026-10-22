#!/usr/bin/env python3
"""Selected option B: distance, five-second alternation, 30-minute expiry."""
from pathlib import Path
from copy import deepcopy
import xml.etree.ElementTree as E
import subprocess
HERE=Path(__file__).resolve().parent
NS='http://www.w3.org/2000/svg';E.register_namespace('',NS)
def tag(name):return '{'+NS+'}'+name
def text(parent,value,**attrs):
 node=E.SubElement(parent,tag('text'),{k.replace('_','-'):str(v) for k,v in attrs.items()});node.text=value;return node
root=E.parse(HERE.parent/'tile-none.svg').getroot();root.set('id','weather-screen')
g=root.find(tag('g'));tile=E.SubElement(g,tag('g'),id='lightning-tile')
E.SubElement(tile,tag('rect'),x='216',y='140',width='96',height='43',rx='5',fill='#142737')
text(tile,'LIGHTNING · 10m',x=223,y=153,font_size=7.2,font_weight=600,fill='#a2b4c9')
E.SubElement(tile,tag('path'),d='M229 157 L223 168 H228 L226 178 L237 164 H231 L234 157 Z',fill='#f3bd62')
text(tile,'4',id='light-count',x=239,y=177,font_size=22,font_weight=600,fill='#f3bd62')
text(tile,'Last 8 mi',id='light-distance',x=259,y=165.5,font_size=7.2,fill='#e7f0f8')
text(tile,'42s ago',id='light-age',x=259,y=178,font_size=6.6,fill='#a2b4c9')
E.ElementTree(root).write(HERE/'lightning.svg',encoding='unicode',xml_declaration=True)
baro=deepcopy(root)
next(n for n in baro.iter(tag('g')) if n.get('id')=='lightning-tile').set('display','none')
E.ElementTree(baro).write(HERE/'barometer.svg',encoding='unicode',xml_declaration=True)
sheet=E.Element(tag('svg'),width='1120',height='535',viewBox='0 0 1120 535')
E.SubElement(sheet,tag('rect'),width='1120',height='535',fill='#edf1f5')
text(sheet,'OPTION B · BAROMETER / LIGHTNING',x=28,y=36,font_size=24,font_weight=700,fill='#142234',font_family='DejaVu Sans,sans-serif')
text(sheet,'Alternates every 5 seconds · stops after 30 minutes without a lightning-data change',x=28,y=63,font_size=14,fill='#536579',font_family='DejaVu Sans,sans-serif')
for x,title,native in [(28,'Barometer phase',baro),(590,'Lightning phase · includes last-strike distance',root)]:
 text(sheet,title,x=x,y=102,font_size=16,font_weight=600,fill='#142234',font_family='DejaVu Sans,sans-serif')
 container=E.SubElement(sheet,tag('g'),transform=f'translate({x} 116) scale(1.55)')
 for node in native:
  if node.tag!=tag('title'):container.append(deepcopy(node))
text(sheet,'No sensor, quiet, or 30-minute expiry: Barometer only. Repeated identical packets do not reset the timer.',x=28,y=513,font_size=13,fill='#536579',font_family='DejaVu Sans,sans-serif')
E.ElementTree(sheet).write(HERE/'proposal.svg',encoding='unicode',xml_declaration=True)
subprocess.run(['rsvg-convert','-o',str(HERE/'proposal.png'),str(HERE/'proposal.svg')],check=True)
svg=E.tostring(root,encoding='unicode')
html='''<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><title>WX Link · Selected lightning concept</title><style>
*{box-sizing:border-box}body{background:#edf1f5;color:#142234;font:16px system-ui;margin:0}main{max-width:850px;margin:auto;padding:24px}h1{font-size:26px;margin:0}p{line-height:1.5}.stage{max-width:640px;margin:20px auto}.stage svg{width:100%;height:auto;display:block}button{font:inherit;padding:12px;border:1px solid #9caec0;border-radius:8px;background:white;cursor:pointer}nav{display:flex;flex-wrap:wrap;gap:8px}.note{background:white;padding:15px;border-left:4px solid #7891a6}#live-status{font-variant-numeric:tabular-nums}label{display:inline-flex;align-items:center;gap:8px;padding:14px 0}a{color:#254e7f}</style><main><h1>Option B · Selected lightning concept</h1><p>Five seconds of lightning, five seconds of Barometer while lightning activity is recent. The lightning tile includes the last-strike distance estimate. Light/UV and NWS alerts keep their original space.</p><div class="stage">'''+svg+'''</div><p id="live-status" class="note" role="status"></p><nav><button id="new-strike">Simulate new strike</button><button id="same-packet">Repeat unchanged packet</button><button id="distance-change">Change last-strike distance</button><button id="age-29">29 minutes unchanged</button><button id="age-30">30 minutes unchanged</button><button id="distance-unknown">Distance unknown</button><button id="metric">Distance: miles</button></nav><label><input type="checkbox" id="sensor-present" checked>Simulate a station that reports lightning</label><p class="note">Fictional readings only. After 30 minutes with no validated lightning-data change, the tile is considered stale for Home and stops rotating. Receiving the same counter/distance does not keep it alive. Stations without lightning never lose Barometer.</p><p>Distance examples here are illustrative. Atlas protocol notes identify miles; metric mode converts to kilometers. Reserved/unknown distance codes still need validation before firmware use. A first counter sample is a baseline, not a new strike. This is a local interactive concept, not installed firmware. <a href="README.md">Selected behavior notes</a> · <a href="proposal.png">Comparison image</a></p></main><script>
const expiry=30*60*1000,period=5000;let clockStart=performance.now(),clockOffset=0,lastChange=-42000,lastStrike=-42000,phaseStart=performance.now(),events=[-180000,-120000,-90000,-42000],distanceMi=8,metric=false;
function now(){return clockOffset+performance.now()-clockStart}function present(){return document.getElementById('sensor-present').checked}
function update(){const time=now(),elapsed=time-lastChange,eventAge=time-lastStrike,eligible=present()&&elapsed<expiry,phase=eligible&&Math.floor((performance.now()-phaseStart)/period)%2===0;document.getElementById('lightning-tile').style.display=phase?'':'none';const shortAge=eventAge<60000?Math.floor(eventAge/1000)+'s':Math.floor(eventAge/60000)+'m';document.getElementById('light-count').textContent=events.filter(t=>time-t<600000).length;document.getElementById('light-distance').textContent=distanceMi===null?'Last —':'Last '+(metric?(distanceMi*1.609344).toFixed(1)+' km':distanceMi+' mi');document.getElementById('light-age').textContent=shortAge+' ago';const changeAge=Math.floor(elapsed/60000)+'m '+Math.floor(elapsed%60000/1000)+'s';document.getElementById('live-status').textContent=!present()?'No lightning sensor reported → Barometer only.':!eligible?'Lightning unchanged for 30 minutes → stale for Home; Barometer only.':(phase?'Lightning':'Barometer')+' phase · last lightning-data change '+changeAge+' ago · swaps every 5 seconds.';}
function changed(){lastChange=now();phaseStart=performance.now();update()}function ageData(ms){clockOffset+=ms-(now()-lastChange);phaseStart=performance.now();update()}
document.getElementById('new-strike').onclick=()=>{lastStrike=now();events.push(lastStrike);changed()};document.getElementById('same-packet').onclick=update;document.getElementById('distance-change').onclick=()=>{distanceMi=distanceMi===8?6:8;changed()};document.getElementById('distance-unknown').onclick=()=>{distanceMi=null;update()};document.getElementById('age-29').onclick=()=>ageData(29*60*1000);document.getElementById('age-30').onclick=()=>ageData(expiry);document.getElementById('sensor-present').onchange=update;document.getElementById('metric').onclick=()=>{metric=!metric;document.getElementById('metric').textContent=metric?'Distance: kilometers':'Distance: miles';update()};setInterval(update,200);update();

</script></html>'''
(HERE/'index.html').write_text(html)
