/*
**
** version.h
** ZenIRCd
** $Id$
*/
#ifndef __versioninclude
#define __versioninclude

/*
 * Utility macros to convert version number constants to strings.
 * Delayed expansion requires two macros to work. See:
 *
 * $ info '(cpp) Stringification'
 */
#define _macro_to_str(n) #n
#define macro_to_str(n)  _macro_to_str(n)


/* 
 * Mark of settings
 */
#ifdef DEBUGMODE
 #define DEBUGMODESET "+(debug)"
#else
 #define DEBUGMODESET ""
#endif
/**/
#ifdef DEBUG
 #define DEBUGSET "(Debug)"
#else
 #define DEBUGSET ""
#endif
/**/
#define COMPILEINFO DEBUGMODESET DEBUGSET

/* Version info follows
 * Please be sure to update ALL fields when changing the version.
 * Also don't forget to bump the protocol version every release.
 */

/**
 * The following code concerns ZEN_VERSION_GENERATION,
 * ZEN_VERSION_MAJOR, and ZEN_VERSION_MINOR.
 *
 * These ZEN_VERSION_* macros can be used so (3rd party) modules
 * can easily distinguish versions.
 *
 * They are set during ./configure, so update ./configure.ac's AC_INIT
 * line upon a new release.
 */

/** Year + week of the year (ISO week number, with Monday as first day of week)
 * Can be useful if the above 3 versionids are insufficient for you (eg: you want to support CVS).
 * This is updated automatically on the CVS server every Monday. so don't touch it.
 */
#define ZEN_VERSION_TIME 202610

#define ZEN_VERSION ((ZEN_VERSION_GENERATION << 24) + (ZEN_VERSION_MAJOR << 16) + (ZEN_VERSION_MINOR << 8))
#define ZenProtocol 6100
#define PATCH1         macro_to_str(ZEN_VERSION_GENERATION)
#define PATCH2         "." macro_to_str(ZEN_VERSION_MAJOR)
#define PATCH3         "." macro_to_str(ZEN_VERSION_MINOR)
#define PATCH4         ZEN_VERSION_SUFFIX
#define PATCH5         ""
#define PATCH6         ""
#define PATCH7         ""
#define PATCH8         COMPILEINFO
#define PATCH9         ""
/* release header */
#define Rh          BASE_VERSION
#define VERSIONONLY PATCH1 PATCH2 PATCH3 PATCH4 PATCH5 PATCH6 PATCH7
#endif /* __versioninclude */
