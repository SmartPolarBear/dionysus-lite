#include "drivers/console/console.h"
#include "drivers/console/cga.h"
#include "drivers/debug/kdebug.h"
#include "lib/libc/stdlib.h"
#include "lib/libc/string.h"
#include "sys/types.h"

#include <stdarg.h>

using putc_function_type = void (*)(uint32_t);
using setpos_function_type = void (*)(size_t);
using setcolor_function_type = void (*)(uint8_t fr, uint8_t bk);

const size_t COLORTABLE_LEN = 8;
struct console_dev
{
    putc_function_type putc;
    setpos_function_type setpos;
    setcolor_function_type setcolor;
    // R G B
    uint8_t colortable[16];
} console_devs[] = {
    [0] = {
        //CGA
        console::cga_putc,
        console::cga_setpos,
        reinterpret_cast<setcolor_function_type>(console::cga_setcolor),
        {[0] = console::CGACOLOR_RED,
         [1] = console::CGACOLOR_GREEN,
         [2] = console::CGACOLOR_BLUE,
         [3] = console::CGACOLOR_LIGHT_BROWN,
         [4] = console::CGACOLOR_MAGENTA,
         [5] = console::CGACOLOR_CYAN,
         [6] = console::CGACOLOR_LIGHT_GREY,
         [7] = console::CGACOLOR_BLACK}}};

static inline void console_putc_impl(uint32_t c)
{
    for (auto dev : console_devs)
    {
        if (dev.putc)
        {
            dev.putc(c);
        }
    }
}

static inline void console_setpos_impl(size_t pos)
{
    for (auto dev : console_devs)
    {
        if (dev.setpos)
        {
            dev.setpos(pos);
        }
    }
}

static inline void console_setcolor_impl(uint8_t fridx, uint8_t bkidx)
{
    for (auto dev : console_devs)
    {
        if (dev.setcolor)
        {
            dev.setcolor(dev.colortable[fridx], dev.colortable[bkidx]);
        }
    }
}

void console::puts(const char *str)
{
    while (*str != '\0')
    {
        console_putc_impl(*str);
        ++str;
    }
}

// buffer for converting ints with itoa
constexpr size_t MAXNUMBER_LEN = 128;
char nbuf[MAXNUMBER_LEN] = {};

void console::printf(const char *fmt, ...)
{
    va_list ap;
    int i, c; //, locking;
    const char *s;

    va_start(ap, fmt);

    //TODO: acquire the lock

    if (fmt == 0)
    {
        KDEBUG_GENERALPANIC("Invalid format strings");
    }

    for (i = 0; (c = fmt[i] & 0xff) != 0; i++)
    {
        if (c != '%')
        {
            console_putc_impl(c);
            continue;
        }
        c = fmt[++i] & 0xff;
        if (c == 0)
            break;
        switch (c)
        {
        case 'c':
        case 'd':
            memset(nbuf, 0, sizeof(nbuf));
            itoa(nbuf, va_arg(ap, int), 10);
            for (size_t i = 0; nbuf[i]; i++)
            {
                console_putc_impl(nbuf[i]);
            }
            break;
        case 'x':
            memset(nbuf, 0, sizeof(nbuf));
            itoa(nbuf, va_arg(ap, int), 16);
            for (size_t i = 0; nbuf[i]; i++)
            {
                console_putc_impl(nbuf[i]);
            }
            break;
        case 'p':
            memset(nbuf, 0, sizeof(nbuf));
            itoa(nbuf, va_arg(ap, size_t), 16);
            for (size_t i = 0; nbuf[i]; i++)
            {
                console_putc_impl(nbuf[i]);
            }
            break;
        case 's':
            if ((s = va_arg(ap, char *)) == 0)
                s = "(null)";
            for (; *s; s++)
                console_putc_impl(*s);
            break;
        case '%':
            console_putc_impl('%');
            break;
        default:
            // Print unknown % sequence to draw attention.
            console_putc_impl('%');
            console_putc_impl(c);
            break;
        }
    }

    va_end(ap);

    //TODO : release the lock
}

void console::console_init(void)
{
}

void console::console_setpos(size_t pos)
{
    console_setpos_impl(pos);
}

void console::console_settextattrib(size_t attribs)
{
    // color flags are ranged from 1<<0 to 1<<5
    if ((0b111111) & attribs) //check if any color flags
    {
        uint8_t fridx = 0, bkidx = 0;
        for (uint8_t i = 0; i < uint8_t(COLORTABLE_LEN); i++) // from 0 to COLORTABLE_LEN, test foreground
        {
            if ((1 << i) & attribs)
            {
                fridx = i;
                break;
            }
        }

        for (uint8_t i = uint8_t(COLORTABLE_LEN);
             i < uint8_t(COLORTABLE_LEN) + uint8_t(COLORTABLE_LEN);
             i++) // from COLORTABLE_LEN to 2*COLORTABLE_LEN-1, test background
        {
            if ((1 << i) & attribs)
            {
                bkidx = i;
                break;
            }
        }

        //set
        console_setcolor_impl(fridx % COLORTABLE_LEN, bkidx % COLORTABLE_LEN);
    }
}