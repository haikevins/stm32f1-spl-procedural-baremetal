#ifndef APP_H
#define APP_H

#ifdef __cplusplus
extern "C" {
#endif

/** Initialize application-level state and modules. */
void App_Init(void);

/** Execute one non-blocking application iteration. */
void App_Run(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_H */
