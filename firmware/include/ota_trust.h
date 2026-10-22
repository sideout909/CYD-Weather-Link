#pragma once
// Public publisher key only. Private key is never stored in firmware/source.
namespace ota_trust {
inline constexpr const char* key_id="c82a512c944bede885f6796244974097";
inline constexpr const char public_key[]=R"KEY(-----BEGIN PUBLIC KEY-----
MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEblm6xQwANAo76xGH6m8NcX41rN3h
K4jk1p5yKsJI1M0p/umv/p92HRrYmwkdB2djoGSBzbvDHb8xjPzdtd5p3Q==
-----END PUBLIC KEY-----
)KEY";
}
