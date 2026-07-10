```text
stm32f1-spl-baremetal-template/
├── app/                          # Application logic and main program flow
│   ├── inc/
│   │   └── app.h
│   └── src/
│       ├── app.c
│       └── main.c
│
├── bsp/                          # Board Support Package for board-level hardware
│   ├── inc/
│   │   ├── bsp.h
│   │   ├── bsp_led.h
│   │   ├── bsp_button.h
│   │   └── bsp_uart.h
│   └── src/
│       ├── bsp.c
│       ├── bsp_led.c
│       ├── bsp_button.c
│       └── bsp_uart.c
│
├── drivers/                      # Drivers for external devices and sensors
│   ├── inc/
│   └── src/
│
├── lib/                          # Reusable utility libraries
│   ├── inc/
│   └── src/
│
├── middleware/                   # RTOS, file systems, and protocol stacks
│   ├── inc/
│   └── src/
│
├── third_party/                  # Vendor and external source code
│   ├── CMSIS/
│   └── STM32F10x_StdPeriph_Driver/
│       ├── inc/
│       └── src/
│
├── system/                       # Startup, interrupts, and system support
│   ├── inc/
│   │   ├── stm32f10x_conf.h
│   │   └── stm32f10x_it.h
│   ├── src/
│   │   ├── stm32f10x_it.c
│   │   └── syscalls.c
│   └── startup/
│       └── startup_stm32f10x_md_gcc.s
│
├── linker/                       # Linker scripts and memory layout
│   └── STM32F103C8Tx_FLASH.ld
│
├── scripts/                      # Flash, debug, and helper scripts
│   ├── flash.sh
│   ├── debug.sh
│   └── openocd.cfg
│
├── docs/                         # Project documentation
│   └── architecture.md
│
├── Makefile
├── README.md
├── LICENSE
└── .gitignore
```
