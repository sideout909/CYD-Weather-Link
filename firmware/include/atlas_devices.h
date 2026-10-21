#pragma once

// Force-include this before the upstream generated device list. Register only
// acurite_txr (the upstream decoder containing Atlas); other emitted models
// are filtered again by the application. The pinned library's MY_DEVICES path
// expands DEVICES rather than registering its full decoder catalogue.
#define MY_DEVICES 1
#define INCLUDE_RTL_433_DEVICES_H_
#define DEVICES DECL(acurite_txr)
#define NUMOFDEVICES 1
#ifdef __cplusplus
extern "C" {
#endif
#if __has_include("r_device.h")
#include "r_device.h"
#else
typedef struct r_device r_device;
#endif
extern r_device const acurite_txr;
#ifdef __cplusplus
}
#endif
