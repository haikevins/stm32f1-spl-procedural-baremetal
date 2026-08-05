#ifndef APPLICATION_H
#define APPLICATION_H

#include <stdbool.h>

void application_init(void);
void application_process(void);

/*
 * Called with interrupts temporarily masked by system_idle().
 * Returns true only when the application can make progress immediately.
 */
bool application_has_pending_work(void);

#endif /* APPLICATION_H */
