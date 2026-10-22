# Windy connection investigation — October 22, 2026

Stable fix: **0.8.1**, owner-approved after successful use of the 0.8.1-rc.1 candidate. No published 0.8.0
asset is overwritten. Scope is Windy TLS trust and credential-free diagnostics.

## Observed failure

Windy was enabled with station ID and saved password, while fresh live RF,
Wi-Fi and clock were healthy. Status showed Connection unavailable with no
successful upload. An instrumented device attempt established:

- DNS resolved stations.windy.com to 35.241.28.33 through the configured LAN DNS.
  Pi-hole was not blocking that attempt. Do not publish the private DNS IP.
- HTTPClient returned -1 and WiFiClientSecure returned TLS -12288
  (MBEDTLS_ERR_X509_FATAL_ERROR). This occurred before a server credential reply.
- Server chain inspection showed WR3 issued by GTS Root R1, followed by a
  cross-signed R1 issued by GlobalSign. The older ESP-IDF bundle verifier had
  previously failed similarly for the sun/moon provider.

## Change

Windy uses the verified public self-signed GTS Root R1 trust anchor directly,
keeping certificate and hostname checks enabled. Source and fingerprint are in
certs/README.md. Other services keep their existing trust setup. No passwords,
keys, payloads or server response bodies are exposed in diagnostics.

Protected status now records Windy HTTP/TLS codes, last attempt, resolved address
and configured DNS server. The last success time remains separate. DNS failures
have their own status and retain bounded retry backoff; fresh-data and five-minute
minimum scheduling remain in effect. The raw API network addresses are local
connection-health information and should be redacted from public bug reports.

## Validation

SC05_X build and host checks pass. The trusted root was matched to the PC root
store; a PC HTTPS request using only that root reached Windy and received the
expected unauthenticated 401. With the explicit root, the real ESP32 received HTTP 200 Accepted for processing
and recorded its first successful Windy upload. DNS remained unchanged. Only
fresh live Atlas observations were submitted; the owner then confirmed the Windy station was online.
No fabricated or replay observations are used for real-service testing.


The owner confirmed the station online and authorized stable 0.8.1 publication.
Stable packaging also normalizes positive SDK handshake results to zero in the
TLS-error diagnostic. The installed rc.1 already uploads successfully; it was
left running for a later owner OTA update. No additional USB restart was needed.
