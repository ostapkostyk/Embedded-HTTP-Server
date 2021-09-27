#ifndef _DEBUG_H_
#define _DEBUG_H_

#define debug_print(...) do { if (EDEBUG) fprintf(stdout, ##__VA_ARGS__); } while (0)

#endif
