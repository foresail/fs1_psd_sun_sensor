#ifndef DEBUG_H_
#define DEBUG_H_

#ifdef SEGGER_RTT
#include <segger/segger_rtt.h>
#define debugprintf(...) SEGGER_RTT_printf(0, __VA_ARGS__)
#define debugassert(x, file, line) if(!(x)) { SEGGER_RTT_printf(0, "Asserted on line %d in %s!\n", line, file); while(1); }
#else
#define debugprintf(...) do { } while (0)
#define SEGGER_RTT_printf(...) do { } while (0)
#define debugassert(x, file, line) do {} while(0)
#endif

#endif /* DEBUG_H_ */
