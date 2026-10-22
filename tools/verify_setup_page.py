#!/usr/bin/env python3
"""Exercise the actual embedded setup HTML with synthetic responses, no device secrets.

Optional tool dependency: playwright. Uses the installed Brave/Chromium browser.
"""
import json
from pathlib import Path
import re
import shutil
import time

ROOT = Path(__file__).resolve().parents[1]


def main():
    from playwright.sync_api import sync_playwright
    html = re.search(r'R"WXHTML\((.*)\)WXHTML";', (ROOT/'firmware/include/setup_page.h').read_text(), re.S)[1]
    config = dict(ssid='Example Wi-Fi', station_id=679, channel='A', radio_enabled=True,
                  timezone='EST5EDT,M3.2.0,M11.1.0', location_set=False, latitude=0, longitude=0,
                  elevation_m=0, display_theme='midnight',date_format='md',clock_24h=False,display_sleep_mode='never',display_timeout_s=300,
                  wu_enabled=False, weathercloud_enabled=False, aprs_enabled=False,
                  aprs_profile='ham', callsign='', update_repo='sideout909/WX-Link', csrf='synthetic-csrf', wu_key_set=True)
    status = dict(ip='192.0.2.10', local_url='http://wxlink.local', mdns_ready=True, wifi=False, radio_ready=False, source='none', forecast_state='Not configured',
                  alert_state='Not configured', packets=0, free_heap=200000,
                  rain_today_in=.12, rain_partial=True, rain_24h_in=.85, rain_24h_partial=False,
                  temperature_F=55.200000076, gust_mph=8.299999237,
                  display=dict(temperature_F=55.200000076, gust_mph=8.299999237, gust_partial=False,
                               wind_mph=6.19999981, direction_deg=225, humidity=75, lux=0, uv=0),
                  callsign='N0CALL', clock_valid=True, sensor_status='Current',
                  nws_pressure_valid=True, nws_pressure_pa=101325, nws_pressure_station='KTEST')
    end_minute = int(time.time() // 60)
    history_rows = [[end_minute-1439+i, None if 500 <= i < 506 else 50+i/1000,
                     5+i%7, 0 if i < 700 else 15000+i, 75] for i in range(1440)]
    forecast_periods = {}
    now = int(time.time())
    for kind, count, duration in [('daily',14,43200),('hourly',24,3600)]:
        forecast_periods[kind] = [dict(name=('Today' if i==0 else 'Period '+str(i)) if kind=='daily' else 'Hour '+str(i),
            short='Partly Sunny', detail='Full local forecast detail '+str(i)+' <b>plain text</b>',
            wind='SW 5 to 10 mph', temperature_F=65-i, rain_probability=20,
            daytime=True, daytime_known=True, start=now-900+i*duration,
            end=now-900+(i+1)*duration) for i in range(count)]
    alert_entries = []
    freshness = dict(forecast=True, alerts=True)
    api_offline = dict(enabled=False)
    initial_status = dict(hold=True, requests=[])
    saves = []
    update_requests = []
    update_state = dict(current_version='0.3.0', available=False, busy=False, message='Updates off', progress=0)
    archive_requests = []
    archive_failure = dict(enabled=False)
    errors = []
    def route(request):
        path = request.request.url.split('weatherlink.test', 1)[-1]
        files = {'/assets/discovery.js': ('assets/web/discovery.js', 'text/javascript'),
                 '/assets/dashboard.js': ('assets/web/dashboard.js', 'text/javascript'),
                 '/assets/location.js': ('assets/web/location.js', 'text/javascript'),
                 '/assets/leaflet-1.9.4.js': ('assets/web/leaflet/leaflet.js', 'text/javascript'),
                 '/assets/leaflet-1.9.4.css': ('assets/web/leaflet/leaflet.css', 'text/css')}
        if path in files:
            name, mime = files[path]
            request.fulfill(status=200, content_type=mime, body=(ROOT/name).read_text())
            return
        if path == '/':
            request.fulfill(status=200, content_type='text/html', body=html)
            return
        if path == '/api/config' and request.request.method == 'POST':
            assert request.request.headers['x-csrf-token'] == config['csrf']
            saves.append(request.request.post_data_json)
            result = dict(saved=True, restarting=True)
        elif path == '/api/config': result = config
        elif path == '/api/status':
            if initial_status['hold']:
                initial_status['requests'].append(request); return
            if api_offline['enabled']:
                request.fulfill(status=503, content_type='application/json', body='{}'); return
            result = status
        elif path.startswith('/api/history?'):
            from urllib.parse import parse_qs, urlsplit
            q = parse_qs(urlsplit(path).query); offset = int(q['offset'][0])
            assert q['count'] == ['120']
            if 'end' in q: assert q['end'] == [str(end_minute)]
            result = dict(end=end_minute,total=1440,next_offset=min(1440,offset+120),rows=history_rows[offset:offset+120])
        elif path.startswith('/api/forecast?'):
            from urllib.parse import parse_qs, urlsplit
            q = parse_qs(urlsplit(path).query); kind=q['type'][0]; offset=int(q['offset'][0])
            entries=forecast_periods[kind]
            result=dict(total=len(entries),next_offset=min(len(entries),offset+4),periods=entries[offset:offset+4],
                        fresh=freshness['forecast'],fetched=now,issued=now,state='Current' if freshness['forecast'] else 'Cached / refresh pending')
        elif path.startswith('/api/alerts?'):
            from urllib.parse import parse_qs, urlsplit
            q=parse_qs(urlsplit(path).query);offset=int(q['offset'][0])
            result=dict(total=len(alert_entries),next_offset=min(len(alert_entries),offset+1),alerts=alert_entries[offset:offset+1],
                        fresh=freshness['alerts'],checked=now,state='Current' if freshness['alerts'] else 'Connection unavailable',theme=3 if alert_entries else 0,truncated=False,revision=len(alert_entries))
        elif path.startswith('/assets/weather.png?'):
            request.fulfill(status=200,content_type='image/png',body=(ROOT/'assets/weather/sunny.png').read_bytes());return
        elif path == '/api/archive': result = dict(ready=True, clock_valid=True, message='Recording five-minute summaries', records=2, capacity=64, end_interval=5000000, free_bytes=1000000)
        elif path.startswith('/api/archive.csv?'):
            from urllib.parse import parse_qs, urlsplit
            query = parse_qs(urlsplit(path).query)
            assert query['end'] == ['5000000']
            offset = int(query['offset'][0]); archive_requests.append(offset)
            if archive_failure['enabled'] and offset == 32:
                request.fulfill(status=503, content_type='text/plain', body='Unavailable')
                return
            body = 'utc_start,value\n2026-10-21T00:00:00Z,0\n' if offset == 0 else '2026-10-21T00:05:00Z,1\n'
            request.fulfill(status=200, content_type='text/csv', body=body)
            return
        elif path == '/api/update': result = update_state
        elif path in ('/api/update/check', '/api/update/install'):
            assert request.request.method == 'POST' and request.request.headers['x-csrf-token'] == config['csrf']
            update_requests.append(path)
            update_state.update(busy=True, message='Check queued' if path.endswith('/check') else 'Install queued')
            result = dict(queued=True)
        elif path == '/api/scan': result = dict(scanning=False, networks=['Example Wi-Fi', '<untrusted SSID>'])
        else: raise AssertionError(path)
        request.fulfill(status=200, content_type='application/json', body=json.dumps(result))
    artifacts = ROOT/'captures/firmware-validation'
    artifacts.mkdir(parents=True, exist_ok=True)
    with sync_playwright() as p:
        browser = p.chromium.launch(executable_path=shutil.which('brave') or '/opt/brave-bin/brave', headless=True)
        page = browser.new_page(viewport=dict(width=1100, height=900))
        page.on('pageerror', lambda e: errors.append(str(e)))
        page.route('http://weatherlink.test/**', route)
        page.goto('http://weatherlink.test/')
        page.wait_for_function("document.querySelector('#alerts').textContent.includes('Checking device connection')")
        assert 'Wi-Fi unavailable' not in page.locator('#alerts').inner_text()
        page.wait_for_function("document.querySelector('#forecast-status').textContent.includes('Checking device connection')")
        assert 'alert-watch' not in (page.locator('#station-alert').get_attribute('class') or '')
        initial_status['hold']=False
        for pending in initial_status['requests']:
            pending.fulfill(status=200,content_type='application/json',body=json.dumps(status))
        page.wait_for_function("document.querySelector('#connection').textContent==='Setup Wi-Fi active'")
        page.wait_for_function("document.querySelector('#station-callsign').textContent==='N0CALL'")
        page.wait_for_function("document.querySelector('#alerts').textContent.includes('Wi-Fi unavailable')")
        # Reverse arrival order: status completes before the deferred dashboard.
        status['wifi']=True
        early=browser.new_page()
        early.route('http://weatherlink.test/**',route)
        held_script=[]
        early.route('http://weatherlink.test/assets/dashboard.js',lambda request:held_script.append(request))
        early.goto('http://weatherlink.test/',wait_until='commit')
        early.wait_for_function('window.wxStatus?.wifi===true')
        assert held_script
        held_script[0].fulfill(status=200,content_type='application/javascript',body=(ROOT/'assets/web/dashboard.js').read_text())
        early.wait_for_function("document.querySelector('#station-callsign').textContent==='N0CALL'")
        early.wait_for_function("document.querySelector('#alerts').textContent.includes('No active alerts')")
        assert 'Wi-Fi unavailable' not in early.locator('#alerts').inner_text()
        early.close()
        status['wifi']=False
        assert page.locator('#local-address a').nth(0).get_attribute('href')=='http://wxlink.local/'
        assert page.locator('#local-address a').nth(1).get_attribute('href')=='http://192.0.2.10/'
        status['mdns_ready']=False
        page.evaluate('status()')
        page.wait_for_function("document.querySelector('#local-address').textContent.includes('Local name unavailable; use IP')")
        status['mdns_ready']=True
        page.evaluate('status()')
        page.wait_for_function("!document.querySelector('#local-address').textContent.includes('unavailable')")
        assert page.locator('#temperature').inner_text() == '55.2 °F'
        assert page.locator('#gust').inner_text() == '8.3 mph'
        assert page.locator('#wind').inner_text() == '6.2 mph'
        assert page.locator('#humidity').inner_text() == '75 %'
        assert page.locator('#lux').inner_text() == '0 lx'
        assert page.locator('#pressure').inner_text() == '29.92 inHg'
        page.wait_for_function("document.querySelectorAll('#forecast-periods article').length===14")
        assert page.locator('#forecast-periods .forecast-art img').count()==14
        assert 'partly-cloudy' in page.locator('#forecast-periods .forecast-art img').first.get_attribute('src')
        assert '<b>plain text</b>' in page.locator('#forecast-periods').inner_text()
        assert page.locator('#forecast-periods b').count() == 0
        page.wait_for_function("document.querySelector('#history-chart path')!==null")
        assert page.locator('#history-chart path').get_attribute('d').count('M') == 2
        assert page.locator('#home-trend path').get_attribute('d').count('M') == 1
        page.locator('[data-metric="2"]').click()
        assert 'Wind peaks' in page.locator('#trend-status').inner_text()
        page.locator('[data-metric="3"]').click()
        assert '0.0 to' in page.locator('#trend-status').inner_text()
        page.locator('[data-metric="1"]').click()
        page.locator('[data-forecast="hourly"]').click()
        assert page.locator('#forecast-periods article').count() == 24
        page.locator('[data-forecast="daily"]').click()
        assert '55.200000076' not in page.locator('#statusdetail').inner_text()
        assert page.locator('#rain').inner_text() == '0.12 in *'
        assert page.get_by_text('Rain · today', exact=True).is_visible()
        page.screenshot(path=str(artifacts/'setup-desktop.png'), full_page=True)
        page.wait_for_function("!document.querySelector('#archive-download').disabled")
        with page.expect_download() as pending:
            page.locator('#archive-download').click()
        download = pending.value
        archive_file = artifacts/'synthetic-seven-day.csv'
        download.save_as(str(archive_file))
        assert archive_requests == [0, 32]
        assert archive_file.read_text().count('utc_start') == 1
        assert '2026-10-21T00:05:00Z,1' in archive_file.read_text()
        page.wait_for_function("!document.querySelector('#archive-download').disabled")
        archive_failure['enabled'] = True
        page.locator('#archive-download').click()
        page.wait_for_function("document.querySelector('#archive-feedback').textContent==='Download interrupted. Try again.'")
        page.wait_for_function("!document.querySelector('#archive-download').disabled")
        archive_failure['enabled'] = False
        page.get_by_role('button', name='Station & Wi-Fi', exact=True).click()
        assert page.locator('#station_id').input_value() == '679'
        assert page.locator('#station-links span').count()==3
        assert 'Sound' not in page.locator('#station-links').inner_text()
        palettes=json.loads((ROOT/'assets/themes/palettes.json').read_text())
        for theme,palette in palettes.items():
            page.locator('#display_theme').select_option(theme)
            assert page.evaluate('document.documentElement.dataset.theme')==theme
            assert page.evaluate("getComputedStyle(document.documentElement).getPropertyValue('--bg').trim()") == palette['background']
            assert page.evaluate("getComputedStyle(document.documentElement).getPropertyValue('--cyan').trim()") == palette['accent']
            assert page.locator('#temperature').inner_text()=='55.2 °F'
            assert page.locator('#forecast-periods .forecast-art img').first.get_attribute('src').endswith('name=partly-cloudy')
            assert ('theme='+theme) in page.locator('#forecast-periods .forecast-art img').first.get_attribute('src')
            if theme in ('matrix','paper','night'):page.screenshot(path=str(artifacts/('theme-'+theme+'-web.png')),full_page=True)
        page.locator('#display_theme').select_option('night')
        assert page.evaluate("getComputedStyle(document.documentElement).getPropertyValue('--green').trim()")=='#7d875b'
        page.locator('#display_theme').select_option('violet')
        assert page.locator('#display_sleep_mode').input_value()=='never'
        assert page.locator('#display_timeout_s').input_value()=='300'
        page.locator('#date_format').select_option('iso')
        page.locator('#clock_24h').check()
        page.locator('#display_sleep_mode').select_option('off')
        page.locator('#display_timeout_s').select_option('30')
        assert page.locator('#wu_key').input_value() == ''
        assert page.locator('#setup_key').input_value() == ''
        page.get_by_role('button', name='Find networks').click()
        page.wait_for_function("document.querySelector('#networks').children.length===2")
        assert page.locator('#networks option').nth(1).get_attribute('value') == '<untrusted SSID>'
        page.get_by_role('button', name='Updates', exact=True).click()
        page.wait_for_function("document.querySelector('#update-state').textContent.includes('Current 0.3.0')")
        assert not page.locator('[name=update_enabled]').is_checked()
        assert not page.locator('[name=update_auto]').is_checked()
        assert page.locator('#update-install').is_disabled()
        page.locator('#update-check').click()
        page.wait_for_function("document.querySelector('#update-feedback').textContent==='Check queued · 0%'")
        assert page.locator('#update-check').is_disabled()
        assert page.locator('#update_repo').input_value() == 'sideout909/WX-Link'
        update_state.update(busy=False, message='Release lookup failed (HTTP -1)')
        page.evaluate('updateStatus()')
        page.wait_for_function("document.querySelector('#update-feedback').textContent==='Release lookup failed (HTTP -1)'")
        assert not page.locator('#update-check').is_disabled()
        page.locator('#update-check').click()
        page.wait_for_function("document.querySelector('#update-feedback').textContent==='Check queued · 0%'")
        update_state.update(busy=False, available=True, available_version='0.3.1', message='New firmware available')
        page.get_by_role('button', name='Overview', exact=True).click()
        page.get_by_role('button', name='Updates', exact=True).click()
        page.wait_for_function("!document.querySelector('#update-install').disabled")
        assert page.locator('#update-feedback').inner_text() == 'New firmware available'
        page.locator('#update-install').click()
        page.wait_for_function("document.querySelector('#update-feedback').textContent==='Install queued · 0%'")
        update_state.update(message='Downloading release', progress=42)
        page.evaluate('updateStatus()')
        page.wait_for_function("document.querySelector('#update-feedback').textContent==='Downloading release · 42%'")
        update_state.update(busy=False, boot_pending=True, message='Startup checks: Local map asset (HTTP -11)')
        page.evaluate('updateStatus()')
        page.wait_for_function("document.querySelector('#update-feedback').textContent==='Startup checks: Local map asset (HTTP -11)'")
        assert page.locator('#save').is_disabled()
        assert page.locator('#update-check').is_disabled()
        assert page.locator('#update-install').is_disabled()
        update_state.update(busy=False, boot_pending=False, current_version='0.3.1', available=False, message='Updated firmware passed startup checks')
        page.evaluate('updateStatus()')
        page.wait_for_function("document.querySelector('#update-feedback').textContent==='Updated firmware passed startup checks'")
        assert update_requests == ['/api/update/check', '/api/update/check', '/api/update/install']
        page.locator('#update_repo').fill('owner/weather-link')
        page.get_by_role('button', name='Sharing', exact=True).click()
        assert page.locator('#weathercloud_id').is_visible()
        assert page.locator('#settings').evaluate('(f)=>f.elements.wu_id!==undefined && f.elements.station_id!==undefined')
        page.locator('#wu_id').fill('EXAMPLE1')
        config['csrf'] = 'synthetic-after-reboot'  # Saving refreshes the token without losing edits.
        page.locator('#save').click()
        page.wait_for_function("document.querySelector('#feedback').textContent.startsWith('Saved.')")
        assert saves[0]['station_id'] == 679 and saves[0]['wu_id'] == 'EXAMPLE1'
        assert saves[0]['wu_key'] == '' and saves[0]['radio_enabled'] is True
        assert saves[0]['setup_key'] == '' and saves[0]['update_repo'] == 'owner/weather-link'
        assert saves[0]['update_enabled'] is False and saves[0]['update_auto'] is False
        assert saves[0]['display_theme']=='violet'
        assert page.evaluate('document.documentElement.dataset.theme')=='violet'
        assert saves[0]['date_format']=='iso' and saves[0]['clock_24h'] is True
        assert saves[0]['display_sleep_mode']=='off' and saves[0]['display_timeout_s']==30
        config['display_theme']='violet'  # Simulated persisted config for subsequent reloads.
        # A second save in the same tab must work after the device restarts.
        page.wait_for_function("!document.querySelector('#save').disabled", timeout=3000)
        page.locator('#wu_id').fill('EXAMPLE2')
        page.locator('#save').click()
        page.wait_for_function("document.querySelector('#feedback').textContent.startsWith('Saved.')")
        assert len(saves) == 2 and saves[-1]['wu_id'] == 'EXAMPLE2'
        # Saved float elevation must remain valid; invalid fields on another tab
        # must reveal that tab and explain the problem instead of blocking silently.
        page.evaluate("document.querySelector('#elevation_m').value='247.100006'")
        assert page.locator('#elevation_m').evaluate('(el)=>el.checkValidity()')
        page.evaluate("document.querySelector('#station_id').value=''")
        page.locator('#save').click()
        page.wait_for_function("document.querySelector('#station').classList.contains('active')")
        assert 'Sensor ID' in page.locator('#feedback').inner_text()
        assert len(saves) == 2
        page.locator('#station_id').fill('679')
        page.set_viewport_size(dict(width=390, height=844))
        page.get_by_role('button', name='Wiring & health', exact=True).click()
        assert page.evaluate('document.documentElement.scrollWidth <= window.innerWidth')
        page.screenshot(path=str(artifacts/'setup-mobile.png'), full_page=True)
        page.get_by_role('button', name='Overview', exact=True).click()
        assert page.evaluate('document.documentElement.scrollWidth <= window.innerWidth')
        page.screenshot(path=str(artifacts/'dashboard-mobile.png'), full_page=True)
        status['sensor_status'] = 'Data stale'
        page.evaluate('status()')
        page.wait_for_function("document.querySelector('#temperature').classList.contains('stale-reading')")
        saved_display = status['display']
        status['display'] = {}
        page.evaluate('status()')
        page.wait_for_function("document.querySelector('#temperature').textContent==='—'")
        assert page.locator('#gust').inner_text() == '—'
        assert page.locator('#humidity').inner_text() == '—'
        assert page.locator('#wind').inner_text() == '—'
        status['display'] = saved_display
        api_offline['enabled'] = True
        page.evaluate('status()')
        page.wait_for_function("document.querySelector('#source').textContent.includes('Device unavailable')")
        assert 'unverified' in page.locator('#alerts').inner_text()
        api_offline['enabled'] = False
        status['wifi'] = True
        page.evaluate('status()')
        page.wait_for_function("document.querySelector('#station-alert').textContent.includes('No active alerts')")
        page.evaluate("WXDashboard.applyTheme('night')")
        assert page.locator('#station-alert').evaluate('(el)=>getComputedStyle(el).color')=='rgb(125, 135, 91)'
        config['display_theme']='paper'
        alert_entries.append(dict(id='synthetic-warning',event='Tornado Warning',headline='Synthetic warning <script>bad()</script>',description='Official description <b>text</b>',instruction='Test instruction',expires=now+7200,category=3))
        page.reload()
        page.wait_for_function("document.querySelector('#station-alert').textContent.includes('Tornado Warning')")
        assert page.locator('#station-alert').get_attribute('class').find('alert-warning') >= 0
        assert page.evaluate('document.documentElement.dataset.theme')=='paper'
        assert page.locator('#station-alert').evaluate('(el)=>getComputedStyle(el).color')=='rgb(255, 240, 243)'
        assert page.locator('#alerts-card').evaluate('(el)=>getComputedStyle(el).backgroundColor')=='rgb(98, 29, 45)'
        assert page.locator('#alert-details script').count() == 0
        assert page.locator('#alert-details summary').inner_text() == 'Synthetic warning <script>bad()</script>'
        page.wait_for_function("!document.querySelector('#sky').hidden")
        label_box=page.locator('#sky small').bounding_box();icon_box=page.locator('.sky-art').bounding_box()
        assert label_box['y']+label_box['height']<=icon_box['y']+1
        status['display']['uv']=3
        page.evaluate('status()')
        page.wait_for_function("document.querySelector('#uv').textContent==='UV 3 Moderate'")
        status['display']['stale_fields']=32
        page.evaluate('status()')
        page.wait_for_function("document.querySelector('#uv').textContent==='UV —'")
        status['display']['stale_fields']=0
        forecast_periods['hourly'][0]['short']='Mostly Cloudy And Breezy'
        page.reload()
        page.wait_for_function("!document.querySelector('#sky-cloud').hidden")
        assert page.locator('#sky-image').get_attribute('src').endswith('name=sunny')
        forecast_periods['hourly'][0]['daytime']=False
        page.reload()
        page.wait_for_function("!document.querySelector('#sky-cloud').hidden && document.querySelector('#sky-image').src.startsWith('data:')")
        forecast_periods['hourly'][0]['short']='Mostly Clear Then Patchy Fog'
        forecast_periods['daily'][0]['short']='Mostly Clear Then Patchy Fog'
        forecast_periods['daily'][0]['daytime']=False
        page.reload()
        page.wait_for_function("document.querySelector('#sky-image').classList.contains('small-moon') && document.querySelector('#sky-cloud').src.endsWith('name=fog')")
        page.wait_for_function("document.querySelector('#forecast-periods .forecast-art .small-moon') && document.querySelector('#forecast-periods .forecast-art .cloud-overlay').src.endsWith('name=fog')")
        # Late-arriving moon data updates rendered cards; future nights advance phase.
        from datetime import datetime
        from zoneinfo import ZoneInfo
        local_now=datetime.fromtimestamp(now,ZoneInfo('America/New_York'))
        status['sun_day']=int(local_now.strftime('%Y%m%d'))
        status['moon_reference']=int(local_now.replace(hour=12,minute=0,second=0,microsecond=0).timestamp())
        status['moon_illumination']=80
        status['moon_phase']='Waxing Gibbous'
        old_moon=page.locator('#forecast-periods .forecast-art img').first.get_attribute('src')
        page.evaluate('status()')
        page.wait_for_function("old=>document.querySelector('#forecast-periods .forecast-art img').src!==old",arg=old_moon)
        forecast_periods['daily'][2]['short']='Mostly Clear'
        forecast_periods['daily'][2]['daytime']=False
        page.reload()
        page.wait_for_function("document.querySelectorAll('#forecast-periods .forecast-art img').length>=14 && document.querySelector('#station-callsign').textContent==='N0CALL'")
        current_moon=page.locator('#forecast-periods .forecast-art img').nth(0).get_attribute('src')
        future_moon=page.locator('#forecast-periods .forecast-art img').nth(3).get_attribute('src')
        assert current_moon.startswith('data:') and future_moon.startswith('data:') and current_moon!=future_moon
        status['moon_phase']='Waning Gibbous'
        page.evaluate('status()')
        page.wait_for_function("old=>document.querySelector('#forecast-periods .forecast-art img').src!==old",arg=current_moon)
        freshness['alerts'] = False; freshness['forecast'] = False
        page.reload()
        page.wait_for_function("document.querySelector('#station-alert').textContent.includes('UNVERIFIED')")
        assert page.locator('#station-alert').get_attribute('class').find('alert-warning') >= 0
        page.wait_for_function("document.querySelector('#forecast-status').textContent.includes('Cached / unverified')")
        assert page.locator('#sky').is_hidden()
        config['ssid'] = ''
        page.reload()
        page.wait_for_function("document.querySelector('#station').classList.contains('active')")
        assert page.locator('#onboarding').is_visible()
        assert 'Set up your weather station' in page.locator('#onboarding').inner_text()
        assert not page.locator('#get-connected').evaluate('node => node.hidden')
        config['ssid'] = 'Example Wi-Fi'; config['location_set'] = True
        page.reload()
        page.wait_for_function("document.querySelector('#onboarding').hidden")
        assert page.locator('#overview').is_visible()
        assert page.locator('#get-connected').is_hidden()
        assert page.locator('nav [data-tab=station]').is_visible()
        assert not errors, errors
        browser.close()
    print('Setup browser checks passed: local hostname/IP and unavailable-name fallback, seven shared palettes, preview/save/reload, Paper alert contrast, three link indicators, startup status races, dashboard rounding, live/partial/missing fields, gapped history, complete day/hourly forecasts, alert text safety, stale/offline states, setup-first onboarding, tabs, settings ownership, secret masking, scan text, CSRF save, repeated saves, cross-tab validation, fractional elevation, mobile layout, no JavaScript errors.')


if __name__ == '__main__': main()
