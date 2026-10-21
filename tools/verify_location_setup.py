#!/usr/bin/env python3
"""Exercise the real embedded map UI with local Leaflet and mocked providers.

Never requests live OSM tiles. Requires the same optional Playwright/Brave
environment as verify_setup_page.py.
"""
import json
from pathlib import Path
import re
import shutil
from urllib.parse import urlparse, parse_qs

ROOT = Path(__file__).resolve().parents[1]


def main():
    from playwright.sync_api import sync_playwright
    html = re.search(r'R"WXHTML\((.*)\)WXHTML";', (ROOT/'firmware/include/setup_page.h').read_text(), re.S)[1]
    config = dict(ssid='Example Wi-Fi', station_id=679, channel='A', radio_enabled=True,
                  timezone='EST5EDT,M3.2.0,M11.1.0', location_set=False, latitude=42.1,
                  longitude=-83.2, elevation_m=300, wu_enabled=False,
                  weathercloud_enabled=False, aprs_enabled=False, aprs_profile='ham',
                  callsign='N0CALL-13', csrf='synthetic-csrf')
    saves, errors, queries, zip_queries, requests = [], [], [], [], []
    behavior = dict(elevation='normal', zip='normal')
    tile = '<svg xmlns="http://www.w3.org/2000/svg" width="256" height="256"><rect width="256" height="256" fill="#dbe6e4"/><path d="M0 128H256M128 0V256" stroke="#91b8b8"/><text x="20" y="60" font-size="14" fill="#264747">TEST MAP TILE</text></svg>'

    def route(request):
        url = urlparse(request.request.url)
        requests.append(url.hostname)
        if url.hostname == 'tile.openstreetmap.org':
            request.fulfill(content_type='image/svg+xml', body=tile)
            return
        if url.hostname == 'api.zippopotam.us':
            zip_queries.append(url.path)
            if behavior['zip'] == 'failure': request.fulfill(status=404, body='{}')
            else: request.fulfill(content_type='application/json', body=json.dumps({'places': [
                {'latitude': '40.7484', 'longitude': '-73.9967', 'place name': '<untrusted place>', 'state abbreviation': 'NY'}]}))
            return
        if url.hostname == 'epqs.nationalmap.gov':
            queries.append(parse_qs(url.query))
            if behavior['elevation'] == 'failure': request.abort()
            else:
                value = None if behavior['elevation'] == 'null' else '-1000000' if behavior['elevation'] == 'nodata' else '123.4'
                request.fulfill(content_type='application/json', body=json.dumps({'value': value}))
            return
        if url.hostname != 'weatherlink.test':
            raise AssertionError('Unexpected external request: '+request.request.url)
        files = {'/assets/dashboard.js': ('assets/web/dashboard.js', 'text/javascript'),
                 '/assets/location.js': ('assets/web/location.js', 'text/javascript'),
                 '/assets/leaflet-1.9.4.js': ('assets/web/leaflet/leaflet.js', 'text/javascript'),
                 '/assets/leaflet-1.9.4.css': ('assets/web/leaflet/leaflet.css', 'text/css')}
        if url.path in files:
            name, mime = files[url.path]
            request.fulfill(content_type=mime, body=(ROOT/name).read_text())
        elif url.path == '/': request.fulfill(content_type='text/html', body=html)
        elif url.path == '/api/config' and request.request.method == 'POST':
            assert request.request.headers['x-csrf-token'] == config['csrf']
            saves.append(request.request.post_data_json)
            request.fulfill(content_type='application/json', body='{"saved":true}')
        elif url.path == '/api/config': request.fulfill(content_type='application/json', body=json.dumps(config))
        elif url.path == '/api/status': request.fulfill(content_type='application/json', body='{"wifi":true,"source":"none"}')
        elif url.path == '/api/history': request.fulfill(status=503,content_type='application/json',body='{"error":"Waiting for clock"}')
        elif url.path == '/api/forecast': request.fulfill(content_type='application/json',body='{"periods":[],"total":0,"next_offset":0,"state":"Not configured","fresh":false}')
        elif url.path == '/api/alerts': request.fulfill(content_type='application/json',body='{"alerts":[],"total":0,"next_offset":0,"state":"Not configured","fresh":false}')
        elif url.path == '/api/archive': request.fulfill(content_type='application/json', body='{"ready":true,"clock_valid":true,"records":0,"capacity":2016,"free_bytes":1000000,"message":"Recording five-minute summaries"}')
        else: raise AssertionError(url.path)

    artifacts = ROOT/'captures/firmware-validation'
    with sync_playwright() as p:
        browser = p.chromium.launch(executable_path=shutil.which('brave') or '/opt/brave-bin/brave', headless=True)
        page = browser.new_page(viewport=dict(width=1100, height=1000))
        page.on('pageerror', lambda e: errors.append(str(e)))
        page.route('**/*', route)
        page.goto('http://weatherlink.test/')
        page.get_by_role('button', name='Station & Wi-Fi', exact=True).click()
        page.wait_for_function("document.querySelector('#latitude').value==='42.1' && !!window.WXLocation")
        assert set(requests) == {'weatherlink.test'}, 'Map must not contact providers on page load'
        page.locator('#zip').fill('bad')
        page.locator('#zip-search').click()
        assert 'five digits' in page.locator('#map-message').inner_text()
        assert not zip_queries
        page.locator('#zip').fill('10001-1234')
        page.locator('#zip-search').click()
        page.wait_for_function("!document.querySelector('#map-shell').hidden && document.querySelector('#map-message').textContent.includes('Approximate area')")
        assert zip_queries == ['/us/10001']
        assert page.locator('#latitude').input_value() == '42.1'
        assert not page.locator('[name=location_set]').is_checked()
        assert page.locator('#map-use').is_disabled()
        assert page.locator('#map-message untrusted').count() == 0
        page.locator('#map-center').click()
        page.wait_for_function("!document.querySelector('#map-use').disabled")
        assert queries[-1]['x'] == ['-73.9967'] and queries[-1]['y'] == ['40.7484']
        assert queries[-1]['units'] == ['Meters']
        assert '123.4 m' in page.locator('#map-elevation').inner_text()
        assert page.locator('#elevation_m').input_value() == '300'
        page.locator('#map-use').click()
        assert page.locator('#latitude').input_value() == '40.74840'
        assert page.locator('#longitude').input_value() == '-73.99670'
        assert page.locator('#elevation_m').input_value() == '123.4'
        assert page.locator('[name=location_set]').is_checked()
        assert not page.locator('[name=aprs_enabled]').is_checked()
        assert page.locator('#callsign').input_value() == 'N0CALL-13'
        page.screenshot(path=str(artifacts/'location-map-desktop.png'), full_page=True)
        # A different dropped point with missing elevation must not inherit 123.4 m.
        for failure in ['null', 'nodata', 'failure']:
            behavior['elevation'] = failure
            box = page.locator('#location-map').bounding_box()
            page.mouse.click(box['x']+box['width']*.6, box['y']+box['height']*.55)
            page.wait_for_function("!document.querySelector('#map-use').disabled")
            assert 'unavailable' in page.locator('#map-elevation').inner_text()
            page.locator('#map-use').click()
            assert page.locator('#elevation_m').input_value() == ''
            assert not page.locator('#elevation_m').evaluate('(e)=>e.checkValidity()')
            page.locator('#elevation_m').fill('245.6')
        page.set_viewport_size(dict(width=390, height=844))
        page.locator('#map-open').click()
        page.wait_for_function("document.querySelectorAll('.leaflet-control-zoom').length===1")
        assert page.evaluate('document.documentElement.scrollWidth <= window.innerWidth')
        page.screenshot(path=str(artifacts/'location-map-mobile.png'), full_page=True)
        # Offline map tools never prevent manual setup or change sharing settings.
        behavior['zip'] = 'failure'
        page.locator('#zip-search').click()
        page.wait_for_function("document.querySelector('#map-message').textContent.startsWith('ZIP lookup failed')")
        page.locator('#map-close').click()
        page.locator('#latitude').fill('42.12345');page.locator('#longitude').fill('-83.12345')
        page.locator('#elevation_m').fill('245.6')
        page.locator('#save').click()
        page.wait_for_function("document.querySelector('#feedback').textContent.startsWith('Saved.')")
        assert saves[-1]['latitude'] == 42.12345 and saves[-1]['elevation_m'] == 245.6
        assert saves[-1]['callsign'] == 'N0CALL-13' and saves[-1]['aprs_enabled'] is False
        assert not errors, errors
        browser.close()
    print('Location checks passed: lazy local map, ZIP+4 centering, explicit point confirmation, elevation units/failure, manual fallback, identity/sharing preservation, phone layout.')


if __name__ == '__main__': main()
