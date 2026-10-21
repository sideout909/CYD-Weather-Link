#!/usr/bin/env python3
"""Exercise the actual embedded setup HTML with synthetic responses, no device secrets.

Optional tool dependency: playwright. Uses the installed Brave/Chromium browser.
"""
import json
from pathlib import Path
import re
import shutil

ROOT = Path(__file__).resolve().parents[1]


def main():
    from playwright.sync_api import sync_playwright
    html = re.search(r'R"WXHTML\((.*)\)WXHTML";', (ROOT/'firmware/include/setup_page.h').read_text(), re.S)[1]
    config = dict(ssid='Example Wi-Fi', station_id=679, channel='A', radio_enabled=True,
                  timezone='EST5EDT,M3.2.0,M11.1.0', location_set=False, latitude=0, longitude=0,
                  elevation_m=0, wu_enabled=False, weathercloud_enabled=False, aprs_enabled=False,
                  aprs_profile='ham', callsign='', update_repo='sideout909/CYD-Weather-Link', csrf='synthetic-csrf', wu_key_set=True)
    status = dict(wifi=False, radio_ready=False, source='none', forecast_state='Not configured',
                  alert_state='Not configured', packets=0, free_heap=200000,
                  rain_today_in=.12, rain_partial=True, rain_24h_in=.85, rain_24h_partial=False)
    saves = []
    update_requests = []
    update_state = dict(current_version='0.3.0', available=False, busy=False, message='Updates off', progress=0)
    archive_requests = []
    archive_failure = dict(enabled=False)
    errors = []
    def route(request):
        path = request.request.url.split('weatherlink.test', 1)[-1]
        files = {'/assets/location.js': ('assets/web/location.js', 'text/javascript'),
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
        elif path == '/api/status': result = status
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
        page.wait_for_function("document.querySelector('#connection').textContent==='Setup Wi-Fi active'")
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
        assert page.locator('#update_repo').input_value() == 'sideout909/CYD-Weather-Link'
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
        assert not errors, errors
        browser.close()
    print('Setup browser checks passed: tabs, settings ownership, secret masking, scan text, CSRF save, repeated saves, cross-tab validation, fractional elevation, mobile layout, no JavaScript errors.')


if __name__ == '__main__': main()
