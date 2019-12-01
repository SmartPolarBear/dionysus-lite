TOP_SRC = .
include $(TOP_SRC)/Makefile.mk

SETS=$(TOP_SRC)/distrib/sets
SUBDIRS = tools kern drivers

BUILD=./build
BASELIST = $(shell cat $(SETS)/base.list)
BASEOBJS = $(addprefix $(BUILD)/,$(BASELIST))

BINLIST = $(shell cat $(SETS)/bin.list)
BINOBJS =  $(addprefix $(BUILD)/bin/,$(BINLIST))

all: $(BUILD) $(SUBDIRS) $(BUILD)/kernel $(BUILD)/disk.img

BUILDDISTDIRS = $(shell ls -d $(BUILD)/*/)
clean:
	for dir in $(SUBDIRS); do $(MAKE) -C $$dir clean; done

	for dir in $(BUILDDISTDIRS); do rm -rf $$dir; done
	@rm -f $(BUILD)/kernel*
	#@rm -rf $(BUILD)

qemu: all 
	$(QEMU_EXE) -serial mon:stdio $(QEMUOPTS)

vbox: all $(BUILD)/disk.qcow2
	$(VBOXMANAGE) $(VBOXMANAGE_FALGS) $(VBOX_MACHINENAME)

debug: all
	@$(QEMU) -serial mon:stdio $(QEMUOPTS) -S $(QEMUGDB) &
	@sleep 2
	@$(GDB) -q -x ./gdbinit

debug4vsc: all
	@$(QEMU) -serial mon:stdio $(QEMUOPTS) -S $(QEMUGDB) &
	@sleep 2
	
$(BUILD)/kernel: $(BASEOBJS)
	$(LD) $(LDFLAGS) -o $@ $^
	$(OBJDUMP) -S $@ > $(BUILD)/kernel.asm
	$(OBJDUMP) -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $(BUILD)/kernel.sym


$(BUILD)/disk.img: $(TOP_SRC)/disk.img $(BUILD)/kernel $(BUILD)/tools/diskimg/diskimg.py
	python3 $(BUILD)/tools/diskimg/diskimg.py update $(TOP_SRC) $(SETS)/hdimage.list

$(BUILD)/disk.qcow2: $(BUILD)/disk.img $(BUILD)/tools/diskimg/diskimg.py
	python3 $(BUILD)/tools/diskimg/diskimg.py convert $(TOP_SRC) qcow2 $< $@

$(SUBDIRS):
	$(MAKE) -C $@ $(MFLAGS) all

$(BUILD): 
	@echo "[MKDIR] $(BUILD)" 
	@mkdir -p $@

$(MOUNTPOINT): 
	@echo "[MKDIR] $(MOUNTPOINT)" 
	@mkdir -p $@

.PHONY: all clean qemu debug debug4vsc $(SUBDIRS) $(BUILD) $(MOUNTPOINT)