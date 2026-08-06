#ifndef COMPILER_H
#define COMPILER_H

#define COMPILER_NORETURN __attribute__((noreturn))
#define COMPILER_WEAK     __attribute__((weak))
#define COMPILER_USED     __attribute__((used))
#define COMPILER_SECTION(name) __attribute__((section(name)))

#endif /* COMPILER_H */
