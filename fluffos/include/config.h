/* config.h.in. processed by CMake */
#ifndef _CONFIG_H_
#define _CONFIG_H_

#define PROJECT_VERSION "fluffos -16c57df"
#define COMPILER "/usr/bin/c++"
#define CXXFLAGS "Broken"

// Features
/* #undef DEBUG */
/* #undef DEBUGMALLOC */
#ifdef DEBUGMALLOC
#define DEBUGMALLOC_EXTENSIONS
#define CHECK_MEMORY
#endif

#define HAVE_JEMALLOC 1
/* #undef ENABLE_DTRACE */

// System headers
/* #undef HAVE_CRYPT_H */
#define HAVE_DIRENT_H 1
#define HAVE_TIME_H 1
#define HAVE_SIGNAL_H 1
#define HAVE_SYS_RESOURCE_H 1
/* #undef HAVE_SYS_RUSAGE_H */
#define HAVE_SYS_TIME_H 1
#define TIME_WITH_SYS_TIME 1
#define HAVE_SYS_STAT_H 1

#endif /* _CONFIG_H_ */
