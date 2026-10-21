#!/usr/bin/env python3
"""Exercise the actual embedded discovery UI with synthetic native candidates."""
import json
from pathlib import Path
import re
import shutil
from urllib.parse import urlsplit

ROOT=Path(__file__).resolve().parents[1]


def main():
    from playwright.sync_api import sync_playwright
    html=re.search(r'R"WXHTML\((.*)\)WXHTML";',(ROOT/'firmware/include/setup_page.h').read_text(),re.S)[1]
    config=dict(ssid='',station_id=679,channel='A',radio_enabled=True,station_confirmed=False,
                timezone='EST5EDT,M3.2.0,M11.1.0',location_set=False,latitude=0,longitude=0,
                elevation_m=0,wu_enabled=False,weathercloud_enabled=False,aprs_enabled=False,
                aprs_profile='ham',callsign='',update_repo='sideout909/CYD-Weather-Link',csrf='fixture-token')
    scan=dict(session=0,active=False,expired=False,elapsed_s=0,duration_s=300,ready=True,
              overflow=0,profile='433.92 MHz OOK',candidates=[])
    saves=[];errors=[];requests=[]
    failures=dict(status=1,start_reply=False)
    def route(request):
        url=urlsplit(request.request.url)
        assert url.hostname=='weatherlink.test','Unexpected external request'
        path=url.path;method=request.request.method;requests.append((method,path))
        if path=='/':request.fulfill(content_type='text/html',body=html);return
        if path.startswith('/assets/'):
            name=path.rsplit('/',1)[-1]
            if name in ['discovery.js','dashboard.js']:
                request.fulfill(content_type='text/javascript',body=(ROOT/'assets/web'/name).read_text())
            else:request.fulfill(content_type='text/javascript',body='')
            return
        if path=='/api/config' and method=='POST':
            assert request.request.headers['x-csrf-token']==config['csrf']
            saves.append(request.request.post_data_json);result=dict(saved=True,restarting=True)
        elif path=='/api/config':result=config
        elif path=='/api/discovery/start':
            assert method=='POST' and request.request.headers['x-csrf-token']==config['csrf']
            scan.update(session=scan['session']+1,active=True,expired=False,elapsed_s=0,candidates=[])
            if failures['start_reply']:
                failures['start_reply']=False
                request.abort();return
            result=dict(started=True)
        elif path=='/api/discovery/cancel':
            assert method=='POST' and request.request.headers['x-csrf-token']==config['csrf']
            scan['active']=False;result=dict(cancelled=True)
        elif path=='/api/discovery':
            if failures['status']:
                failures['status']-=1
                request.fulfill(status=503,content_type='application/json',body='{"error":"Temporarily unavailable"}');return
            result=scan
        elif path=='/api/status':result=dict(wifi=False,radio_ready=True,source='none',station_confirmed=False,sensor_status='Waiting')
        elif path=='/api/update':result=dict(current_version='0.5.1',busy=False,available=False,message='Off')
        elif path=='/api/archive':result=dict(ready=True,clock_valid=False,records=0,capacity=2016,message='Waiting for clock')
        elif path=='/api/history':request.fulfill(status=503,content_type='application/json',body='{}');return
        elif path=='/api/forecast':result=dict(total=0,next_offset=0,periods=[],fresh=False,state='Not configured')
        elif path=='/api/alerts':result=dict(total=0,next_offset=0,alerts=[],fresh=False,state='Not configured')
        else:raise AssertionError(path)
        request.fulfill(content_type='application/json',body=json.dumps(result))
    with sync_playwright() as p:
        browser=p.chromium.launch(executable_path=shutil.which('brave') or '/opt/brave-bin/brave',headless=True)
        page=browser.new_page(viewport=dict(width=390,height=844))
        page.on('pageerror',lambda error:errors.append(str(error)));page.route('**/*',route)
        page.goto('http://weatherlink.test/')
        page.wait_for_function("!!window.WXDiscovery && document.querySelector('#station').classList.contains('active')")
        assert page.locator('#onboarding').is_visible()
        assert not page.locator('#station_id').is_visible()
        assert '5 minutes' in page.locator('#discovery-start').locator('..').locator('..').inner_text()
        page.locator('#ssid').fill('My home Wi-Fi')
        page.locator('#discovery-start').click()
        page.wait_for_function("!document.querySelector('#discovery-cancel').hidden")
        page.wait_for_function("document.querySelector('#discovery-message').textContent.includes('checking scan status again')")
        assert page.locator('#discovery-start').is_disabled()
        page.wait_for_function("document.querySelector('#discovery-message').textContent.startsWith('Listening')")
        assert requests.count(('POST','/api/discovery/start'))==1
        assert not saves
        neighbor=dict(token=1,name='AcuRite Atlas',id=123,channel='B',sightings=1,age_s=0,
                      suggested=False,selectable=False,supported=True,temperature_F=60,humidity=70,wind_mph=0)
        iris=dict(token=2,name='AcuRite Iris / 5-in-1',id=4095,channel='A',sightings=3,age_s=0,
                  suggested=True,selectable=False,supported=False)
        ours=dict(token=3,name='AcuRite Atlas',id=679,channel='A',sightings=3,age_s=0,
                  suggested=True,selectable=True,supported=True,temperature_F=55.200000076,humidity=74,wind_mph=0)
        scan['candidates']=[neighbor,iris,ours]
        page.evaluate('window.WXDiscovery.refresh()')
        page.wait_for_function("document.querySelectorAll('#discovery-candidates .card').length===3")
        assert page.get_by_role('button',name='Use this station',exact=True).count()==1
        assert '55.2 °F' in page.locator('#discovery-candidates').inner_text()
        assert 'Integration for this station family is not available' in page.locator('#discovery-candidates').inner_text()
        assert page.evaluate('document.documentElement.scrollWidth<=innerWidth+1')
        artifacts=ROOT/'captures/firmware-validation';artifacts.mkdir(parents=True,exist_ok=True)
        page.locator('#station').screenshot(path=str(artifacts/'discovery-candidates-mobile.png'))
        page.get_by_role('button',name='Use this station',exact=True).click()
        assert not saves
        assert page.locator('[name=discovery_candidate]').input_value()=='3'
        page.locator('#save').click()
        page.wait_for_function("document.querySelector('#feedback').textContent.includes('Saved.')")
        assert len(saves)==1 and saves[0]['ssid']=='My home Wi-Fi'
        assert saves[0]['discovery_session']==1 and saves[0]['discovery_candidate']==3
        assert saves[0]['station_id']==679 and saves[0]['manual_binding'] is False
        # Cancel clears staged identity; no implicit settings save or replacement.
        page.locator('#discovery-cancel').click()
        page.wait_for_function("document.querySelector('#discovery-message').textContent.includes('cancelled')")
        assert page.locator('[name=discovery_session]').input_value()=='0' and len(saves)==1
        # The POST was accepted, but its reply and first status response were
        # lost. Recover by reading status, without issuing another Start.
        failures.update(start_reply=True,status=1)
        page.locator('#discovery-start').click()
        page.wait_for_function("!document.querySelector('#discovery-cancel').hidden")
        page.wait_for_function("document.querySelector('#discovery-message').textContent.startsWith('Listening')")
        assert requests.count(('POST','/api/discovery/start'))==2
        scan.update(active=False,expired=True,elapsed_s=300,candidates=[])
        page.evaluate('window.WXDiscovery.refresh()')
        page.wait_for_function("document.querySelector('#discovery-start').textContent==='Scan again'")
        assert not page.locator('#discovery-progress').is_hidden()
        assert page.locator('#discovery-progress').get_attribute('max')=='300'
        assert page.evaluate('document.documentElement.scrollWidth<=innerWidth+1')
        assert not errors,errors
        target=ROOT/'captures/firmware-validation/discovery-mobile.png';target.parent.mkdir(parents=True,exist_ok=True)
        page.screenshot(path=str(target),full_page=True)
        browser.close()
    print('Discovery browser checks passed: setup-first, hidden IDs, CSRF, candidates, staged selection, preserved edits, cancel, timeout, mobile width, first-status failure and lost Start reply recovery without duplicate starts.')


if __name__=='__main__':main()
