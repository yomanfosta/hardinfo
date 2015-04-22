#ifndef __CONFIG_H__
#define __CONFIG_H__

#define VERSION "2015-04-21.22:01:49"
#define ARCH_x86_64
#define ARCH     "ARCH_x86_64"
#define PLATFORM "Linux"
#define KERNEL   "3.2.0-57-generic"
#define HOSTNAME "Winterfell"
#define PREFIX "/usr/share/hardinfo/"
#define LIBPREFIX "/usr/lib/hardinfo/"
#define HAS_LINUX_WE
#define DEBUG(msg,...) fprintf(stderr, "*** %s:%d (%s) *** " msg "\n", \
                               __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#define ENABLE_BINRELOC 1
#define RELEASE 0

#endif	/* __CONFIG_H__ */
