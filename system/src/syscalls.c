#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

extern uint8_t _end;
extern uint8_t _estack;
extern uint8_t _Min_Stack_Size;

static uintptr_t heap_end_address;

void *_sbrk(ptrdiff_t increment)
{
    const uintptr_t heap_start = (uintptr_t)&_end;
    const uintptr_t stack_limit = (uintptr_t)&_estack -
                                  (uintptr_t)&_Min_Stack_Size;
    uintptr_t previous_heap_end;

    if (heap_end_address == 0U)
    {
        heap_end_address = heap_start;
    }

    previous_heap_end = heap_end_address;

    if (increment > 0)
    {
        const uintptr_t growth = (uintptr_t)increment;

        if ((growth > stack_limit) ||
            (heap_end_address > (stack_limit - growth)))
        {
            errno = ENOMEM;
            return (void *)-1;
        }

        heap_end_address += growth;
    }
    else if (increment < 0)
    {
        const uintptr_t shrink = (uintptr_t)(-increment);

        if ((heap_end_address < heap_start) ||
            (shrink > (heap_end_address - heap_start)))
        {
            errno = ENOMEM;
            return (void *)-1;
        }

        heap_end_address -= shrink;
    }

    return (void *)previous_heap_end;
}

int _write(int file, const char *buffer, int length)
{
    (void)file;
    (void)buffer;
    return length;
}

int _read(int file, char *buffer, int length)
{
    (void)file;
    (void)buffer;
    (void)length;
    return 0;
}

int _close(int file)
{
    (void)file;
    return -1;
}

int _fstat(int file, struct stat *status)
{
    (void)file;
    status->st_mode = 0;
    return 0;
}

int _isatty(int file)
{
    (void)file;
    return 1;
}

int _lseek(int file, int offset, int whence)
{
    (void)file;
    (void)offset;
    (void)whence;
    return 0;
}

int _getpid(void)
{
    return 1;
}

int _kill(int pid, int signal)
{
    (void)pid;
    (void)signal;
    errno = EINVAL;
    return -1;
}

void _init(void)
{
}

void _fini(void)
{
}

void _exit(int status)
{
    (void)status;

    while (1)
    {
    }
}
