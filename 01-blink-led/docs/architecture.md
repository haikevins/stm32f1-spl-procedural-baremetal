# Blink LED Architecture

This project keeps the template's stable interfaces and startup flow.

```text
main.c                       unchanged template entry point
├── BSP_Init()               board initialization
│   └── BSP_LED_Init()       PC13 active-low LED
├── App_Init()               initialize 1 ms SysTick time base
└── App_Run()                toggle LED after each 500 ms interval
```

The example adds behavior without changing `main.c`, `app.h`, or `bsp.h`.
`App_Run()` remains non-blocking and uses unsigned elapsed-time subtraction so
32-bit millisecond counter wraparound is handled correctly.
