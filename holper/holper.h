#pragma once

#define TO_STRING(s) TO_STRING2(s)
#define TO_STRING2(s) #s

#define HOLPER_VERSION_MAJOR 0
#define HOLPER_VERSION_MINOR 0
#define HOLPER_VERSION_REVISION 4
#define HOLPER_VERSION "v" TO_STRING(HOLPER_VERSION_MAJOR) "." \
  TO_STRING(HOLPER_VERSION_MINOR) "." \
  TO_STRING(HOLPER_VERSION_REVISION)

#ifdef __GNUC__
#define UNUSED(x) UNUSED_ ## x __attribute__((__unused__))
#else
#error wtf are you using?
#endif
