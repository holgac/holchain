#pragma once

/*
#define TO_STRING(s) TO_STRING2(s)
#define TO_STRING2(s) #s
*/

#ifdef __GNUC__
#define UNUSED(x) UNUSED_ ## x __attribute__((__unused__))
#else
#error wtf are you using?
#endif
