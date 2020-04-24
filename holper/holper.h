#pragma once

#define TO_STRING(s) TO_STRING2(s)
#define TO_STRING2(s) #s

#define COMBINE(a,b) COMBINE2(a,b)
#define COMBINE2(a,b) a##b
#define RANDOMNAME(prefix) COMBINE(prefix, __LINE__)

#define HOLPER_VERSION_MAJOR 0
#define HOLPER_VERSION_MINOR 1
#define HOLPER_VERSION_REVISION 0
#define HOLPER_VERSION "v" TO_STRING(HOLPER_VERSION_MAJOR) "." \
  TO_STRING(HOLPER_VERSION_MINOR) "." \
  TO_STRING(HOLPER_VERSION_REVISION)

#ifdef __GNUC__
#define UNUSED(x) UNUSED_ ## x __attribute__((__unused__))
#else
#error wtf are you using?
#endif

typedef unsigned long long u64;
