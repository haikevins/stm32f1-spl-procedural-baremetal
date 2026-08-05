#ifndef BUTTON_SERVICE_H
#define BUTTON_SERVICE_H

#include <stdbool.h>

void button_service_init(void);
void button_service_process(void);
bool button_service_take_pressed_event(void);

#endif /* BUTTON_SERVICE_H */
