#ifndef COMPILER_H
#define COMPILER_H

#define COMPILER_NORETURN __attribute__((noreturn))
#define COMPILER_WEAK     __attribute__((weak))
#define COMPILER_USED     __attribute__((used))
#define COMPILER_SECTION(name) __attribute__((section(name)))

/*
 * Prevent the compiler from moving ordinary memory accesses across this
 * point. On Cortex-M3, the SRAM accesses used by the single-producer /
 * single-consumer ring buffers are naturally ordered by the processor.
 */
#define COMPILER_MEMORY_BARRIER() __asm volatile ("" ::: "memory")

#endif /* COMPILER_H */
