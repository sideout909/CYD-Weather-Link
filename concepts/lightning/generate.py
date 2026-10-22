#!/usr/bin/env python3
"""Native SVG lightning concepts based on the existing Home v2/theme geometry."""
from pathlib import Path
from copy import deepcopy
import xml.etree.ElementTree as ET
import subprocess

HERE=Path(__file__).resolve().parent
NS='http://www.w3.org/2000/svg'
ET.register_namespace('',NS)
def tag(name):return '{'+NS+'}'+name
def text(parent,value,**attrs):
 n=ET.SubElement(parent,tag('text'),{k.replace('_','-'):str(v) for k,v in attrs.items()});n.text=value;return n
base=ET.parse(HERE.parent/'themes/ice-blue.svg').getroot()

def screen(option,state):
 root=deepcopy(base);g=root.find(tag('g'))
 for parent in root.iter():
  for n in list(parent):
   if n.tag==tag('path') and n.get('d','').startswith('M300 30'):parent.remove(n)
 for n in root.iter(tag('text')):
  if n.text=='BARO':n.text='BAROMETER';n.set('font-size','8')
  if n.text=='NWS' and n.get('x')=='140':n.set('x','150');n.set('text-anchor','middle');n.set('y','58')
  if n.text=='10/21 3:42 PM':n.text='Oct 22 2:08 PM';n.set('font-size','8')
 active=state in ('recent','warning')
 stale=state=='stale'
 amber='#f3bd62';muted='#a2b4c9'
 if option=='strip' and (active or stale):
  for parent in root.iter():
   for n in list(parent):
    if n.tag==tag('text') and (n.text or '').startswith('24.0 klx'):parent.remove(n)
  ET.SubElement(g,tag('rect'),x='8',y='122',width='166',height='15',rx='3',fill='#33291c' if active else '#19232e')
  bolt=ET.SubElement(g,tag('path'),d='M17 124 L12 130 H16 L14 135 L22 128 H18 L20 124 Z',fill=amber if active else muted)
  text(g,'4 strikes / 10m · 42s ago' if active else 'Lightning data old',x=26,y=132.5,font_size=8.5,fill=amber if active else muted,font_weight=600)
 if option=='tile' and state!='none':
  for parent in root.iter():
   for n in list(parent):
    if n.tag==tag('text') and (n.get('x') in ('223','282')):parent.remove(n)
  text(g,'LIGHTNING · 10 MIN',x=223,y=153,font_size=7.4,font_weight=600,fill=muted)
  ET.SubElement(g,tag('path'),d='M230 158 L224 168 H229 L227 177 L239 164 H232 L235 158 Z',fill=amber if active else muted)
  text(g,'4' if active else ('—' if stale else '0'),x=244,y=176,font_size=23,font_weight=600,fill=amber if active else '#e7f0f8')
  text(g,'42s ago' if active else ('Data old' if stale else 'No new'),x=272,y=176 if active or stale else 166,font_size=6.5,fill=muted)
  if not active and not stale:text(g,'strikes',x=272,y=177,font_size=6.5,fill=muted)
 if state=='warning':
  for n in root.iter(tag('rect')):
   if n.get('y')=='190':n.set('fill','#4b1b28');n.set('stroke','#925165')
  for n in root.iter(tag('text')):
   if (n.text or '').startswith('NWS ·'):
    n.text='NWS · Severe T-storm Warning';n.set('font-size','10.6');n.set('fill','#fff0f3')
   if (n.text or '').startswith('Checked '):n.text='Tap for warning details';n.set('fill','#f1c3cf')
 root.find(tag('title')).text=f'Fictional lightning concept: {option}, {state}'
 return root

for option in ('strip','tile'):
 for state in ('none','quiet','recent','stale','warning'):
  ET.ElementTree(screen(option,state)).write(HERE/f'{option}-{state}.svg',encoding='unicode',xml_declaration=True)

sheet=ET.Element(tag('svg'),width='1120',height='910',viewBox='0 0 1120 910')
ET.SubElement(sheet,tag('rect'),width='1120',height='910',fill='#edf1f5')
text(sheet,'ATLAS LIGHTNING · HOME CONCEPTS',x=28,y=36,font_size=24,font_weight=700,fill='#142234',font_family='DejaVu Sans,sans-serif')
text(sheet,'Same Home layout · fictional readings · no firmware changes',x=28,y=62,font_size=14,fill='#536579',font_family='DejaVu Sans,sans-serif')
items=[('No lightning sensor · unchanged Home','strip','none',28,103),('A · Recent strikes — recommended','strip','recent',590,103),('A · Data old, not zero strikes','strip','stale',28,514),('B · Optional tile replaces Barometer','tile','recent',590,514)]
for title,option,state,x,y in items:
 text(sheet,title,x=x,y=y,font_size=16,font_weight=600,fill='#142234',font_family='DejaVu Sans,sans-serif')
 group=ET.SubElement(sheet,tag('g'),transform=f'translate({x} {y+13}) scale(1.55)')
 native=screen(option,state)
 for child in native:
  if child.tag!=tag('title'):group.append(deepcopy(child))
ET.ElementTree(sheet).write(HERE/'comparison.svg',encoding='unicode',xml_declaration=True)
subprocess.run(['rsvg-convert','-o',str(HERE/'comparison.png'),str(HERE/'comparison.svg')],check=True)
print('Wrote editable SVGs and rendered concept comparison; no firmware files changed.')
