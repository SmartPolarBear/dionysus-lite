#include "arch/amd64/x86.h"

#include "drivers/acpi/acpi.h"
#include "drivers/acpi/cpu.h"
#include "drivers/apic/apic.h"
#include "drivers/apic/traps.h"
#include "drivers/console/console.h"
#include "drivers/debug/kdebug.h"

#include "sys/memlayout.h"
#include "sys/mmu.h"

namespace pic8259A
{
enum controllers
{
    IO_PIC1 = 0x20,
    IO_PIC2 = 0xA0
};

constexpr size_t IRQ_SLAVE = 2;

static uint16_t irqmask = 0xFFFF & ~(1 << IRQ_SLAVE);

static inline void pic_setmask(uint16_t mask)
{
    irqmask = mask;
    outb(IO_PIC1 + 1, mask);
    outb(IO_PIC2 + 1, mask >> 8);
}

static inline void pic_enable(int irq)
{
    pic_setmask(irqmask & ~(1 << irq));
}

void initialize_pic(void)
{
    // mask all interrupts
    outb(IO_PIC1 + 1, 0xFF);
    outb(IO_PIC2 + 1, 0xFF);

    // Set up master (8259A-1)

    // ICW1:  0001g0hi
    //    g:  0 = edge triggering, 1 = level triggering
    //    h:  0 = cascaded PICs, 1 = master only
    //    i:  0 = no ICW4, 1 = ICW4 required
    outb(IO_PIC1, 0x11);

    // ICW2:  Vector offset
    outb(IO_PIC1 + 1, TRAP_IRQ0);

    // ICW3:  (master PIC) bit mask of IR lines connected to slaves
    //        (slave PIC) 3-bit # of slave's connection to master
    outb(IO_PIC1 + 1, 1 << IRQ_SLAVE);

    // ICW4:  000nbmap
    //    n:  1 = special fully nested mode
    //    b:  1 = buffered mode
    //    m:  0 = slave PIC, 1 = master PIC
    //      (ignored when b is 0, as the master/slave role
    //      can be hardwired).
    //    a:  1 = Automatic EOI mode
    //    p:  0 = MCS-80/85 mode, 1 = intel x86 mode
    outb(IO_PIC1 + 1, 0x3);

    // Set up slave (8259A-2)
    outb(IO_PIC2, 0x11);              // ICW1
    outb(IO_PIC2 + 1, TRAP_IRQ0 + 8); // ICW2
    outb(IO_PIC2 + 1, IRQ_SLAVE);     // ICW3
    // NB Automatic EOI mode doesn't tend to work on the slave.
    // Linux source code says it's "to be investigated".
    outb(IO_PIC2 + 1, 0x3); // ICW4

    // OCW3:  0ef01prs
    //   ef:  0x = NOP, 10 = clear specific mask, 11 = set specific mask
    //    p:  0 = no polling, 1 = polling mode
    //   rs:  0x = NOP, 10 = read IRR, 11 = read ISR
    outb(IO_PIC1, 0x68); // clear specific mask
    outb(IO_PIC1, 0x0a); // read IRR by default

    outb(IO_PIC2, 0x68); // OCW3
    outb(IO_PIC2, 0x0a); // OCW3

    if (irqmask != 0xFFFF)
    {
        pic_setmask(irqmask);
    }
}

} // namespace pic8259A

enum ioapic_regs
{
    IOAPICID = 0x00,
    IOAPICVER = 0x01,
    IOAPICARB = 0x02,
    IOREDTBL_BASE = 0x10,
};

/*
Field	            Bits	    Description
Vector	            0 - 7	    The Interrupt vector that will be raised on the specified CPU(s).
Delivery Mode	    8 - 10	    How the interrupt will be sent to the CPU(s). It can be 000 (Fixed), 001 (Lowest Priority), 010 (SMI), 100 (NMI), 101 (INIT) and 111 (ExtINT). Most of the cases you want Fixed mode, or Lowest Priority if you don't want to suspend a high priority task on some important Processor/Core/Thread.
Destination Mode	11	        Specify how the Destination field shall be interpreted. 0: Physical Destination, 1: Logical Destination
Delivery Status	    12	        If 0, the IRQ is just relaxed and waiting for something to happen (or it has fired and already processed by Local APIC(s)). If 1, it means that the IRQ has been sent to the Local APICs but it's still waiting to be delivered.
Pin Polarity	    13	        0: Active high, 1: Active low. For ISA IRQs assume Active High unless otherwise specified in Interrupt Source Override descriptors of the MADT or in the MP Tables.
Remote IRR	        14	        TODO
Trigger Mode	    15	        0: Edge, 1: Level. For ISA IRQs assume Edge unless otherwise specified in Interrupt Source Override descriptors of the MADT or in the MP Tables.
Mask	            16	        Just like in the old PIC, you can temporary disable this IRQ by setting this bit, and reenable it by clearing the bit.
Destination	        56 - 63	    This field is interpreted according to the Destination Format bit. If Physical destination is choosen, then this field is limited to bits 56 - 59 (only 16 CPUs addressable). You put here the APIC ID of the CPU that you want to receive the interrupt. TODO: Logical destination format...
*/

union redirection_entry {
    struct
    {
        uint64_t vector : 8;
        uint64_t delievery_mode : 3;
        uint64_t destination_mode : 1;
        uint64_t delivery_status : 1;
        uint64_t polarity : 1;
        uint64_t remote_irr : 1;
        uint64_t trigger_mode : 1;
        uint64_t mask : 1;
        uint64_t reserved : 39;
        uint64_t destination_id : 8;
    } __attribute__((packed));
    struct
    {
        uint32_t raw_low;
        uint32_t raw_high;
    } __attribute__((packed));
};

enum delievery_mode
{
    DLM_FIXED = 0,
    DLM_LOWEST_PRIORITY = 1,
    DLM_SMI = 2,
    DLM_NMI = 4,
    DLM_INIT = 5,
    DLM_EXTINT = 7,
};

enum trigger_mode
{
    TRG_EDGE = 0,
    TRG_LEVEL = 1,
};

enum destination_mode
{
    DTM_PHYSICAL = 0,
    DTM_LOGICAL = 1,
};

void write_ioapic(const uintptr_t apic_base, const uint8_t reg, const uint32_t val)
{
    // tell IOREGSEL where we want to write to
    *(volatile uint32_t *)(apic_base) = reg;
    // write the value to IOWIN
    *(volatile uint32_t *)(apic_base + 0x10) = val;
}

uint32_t read_ioapic(const uintptr_t apic_base, const uint8_t reg)
{
    // tell IOREGSEL where we want to read from
    *(volatile uint32_t *)(apic_base) = reg;
    // return the data from IOWIN
    return *(volatile uint32_t *)(apic_base + 0x10);
}
void io_apic::init_ioapic(void)
{
    pic8259A::initialize_pic();

    auto ioapic = acpi::get_first_ioapic();

    uintptr_t ioapic_addr = IO2V(ioapic.addr);

    size_t redirection_count = (read_ioapic(ioapic_addr, IOAPICVER) >> 16) & 0b11111111;
    size_t apicid = (read_ioapic(ioapic_addr, IOAPICID) >> 24) & 0b1111;

    if (apicid != ioapic.id)
    {
        console::printf("WARNING: inconsistence between apicid from IOAPICID register (%d) and ioapic.id (%d)\n", apicid, ioapic.id);
    }

    for (size_t i = 0; i <= redirection_count; i++)
    {
        redirection_entry redir =
            {
                .vector = 0,
                .delievery_mode = DLM_FIXED,
                .destination_mode = DTM_PHYSICAL,
                .polarity = 0,
                .trigger_mode = TRG_EDGE,
                .mask = false,
                .destination_id = 0,
            };

        constexpr auto a = sizeof(decltype(redir));

        write_ioapic(ioapic_addr, IOREDTBL_BASE + i * 2 + 0, redir.raw_low);
        write_ioapic(ioapic_addr, IOREDTBL_BASE + i * 2 + 1, redir.raw_high);
    }
};