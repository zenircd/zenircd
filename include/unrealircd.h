/**
 * Compatibility header for UnrealIRCd third-party modules.
 *
 * ZenIRCd renamed the public include to zenircd.h. Provide unrealircd.h so
 * existing Unreal modules (and the upstream test suite) can still compile.
 */
#ifndef ZENIRCD_UNREALIRCD_COMPAT_H
#define ZENIRCD_UNREALIRCD_COMPAT_H

#include "zenircd.h"

/* Report an UnrealIRCd 6-compatible numeric for #if UNREAL_VERSION checks. */
#ifndef UNREAL_VERSION
#define UNREAL_VERSION 0x06020000
#endif
#ifndef UNREAL_VERSION_GENERATION
#define UNREAL_VERSION_GENERATION 6
#endif
#ifndef UNREAL_VERSION_MAJOR
#define UNREAL_VERSION_MAJOR 2
#endif
#ifndef UNREAL_VERSION_MINOR
#define UNREAL_VERSION_MINOR 0
#endif
#ifndef UNREAL_VERSION_TIME
#define UNREAL_VERSION_TIME ZEN_VERSION_TIME
#endif

#endif /* ZENIRCD_UNREALIRCD_COMPAT_H */
