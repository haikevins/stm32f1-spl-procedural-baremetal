#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdbool.h>

#include "compiler.h"

bool system_init(void);
void system_idle(void);
COMPILER_NORETURN void system_panic(void);

#endif /* SYSTEM_H */
