TARGET = isl-util

CROSS_COMPILE = riscv64-unknown-linux-gnu-
# CC = $(CROSS_COMPILE)aarch64-linux-gnu-gcc
# AS = $(CROSS_COMPILE)aarch64-linux-gnu-gcc
# LD = $(CROSS_COMPILE)aarch64-linux-gnu-gcc
CC = $(CROSS_COMPILE)gcc
AS = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)gcc
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump

OUTDIR = build
SRCDIR = src
INCDIR = src

C_SRC = $(foreach dir, $(SRCDIR), $(wildcard $(dir)/*.c))
S_SRC = $(foreach dir, $(SRCDIR), $(wildcard $(dir)/*.s))
S_SRC += $(foreach dir, $(SRCDIR), $(wildcard $(dir)/*.S))
SRC = $(C_SRC) $(S_SRC)
VPATH = $(sort $(dir $(SRC)))
OBJ = $(addprefix $(OUTDIR)/, $(notdir $(addsuffix .o, $(basename $(SRC)))))

OBJ := $(sort $(OBJ))
DEPS := $(patsubst %.o, %.d, $(OBJ))

OPT_CFLAGS = -g -O2

CFLAGS = $(OPT_CFLAGS) -std=gnu11 -Wall \
	 $(foreach dir, $(INCDIR), -I$(dir))

ASFLAGS = $(OPT_CFLAGS) -x assembler-with-cpp \
          -Wa,--no-warn

LDFLAGS = $(OPT_CFLAGS) -static \
	  -Wl,-Map,$(OUTDIR)/$(TARGET).map -Wl,--gc-sections \
	  $(foreach dir, $(INCDIR), -I$(dir))

ifeq ($(strip $(V)), 1)
	Q =
	E =
else
	Q = @
	E = 1>/dev/null 2>&1
endif

all:$(OUTDIR) \
	$(OUTDIR)/$(TARGET)

$(OUTDIR)/$(TARGET): $(OUTDIR)/$(TARGET).elf
	@echo '[OBJCOPY] $@'
	$(Q)cp $< $@

$(OUTDIR)/%.o: %.c | $(OUTDIR)
	@echo '[CC] $@'
	$(Q)$(CC) $(CFLAGS) -c -o $@ $<

$(OUTDIR)/%.o: %.s | $(OUTDIR)
	@echo '[AS] $@'
	$(Q)$(AS) $(ASFLAGS) -c -o $@ $<

$(OUTDIR)/%.o: %.S | $(OUTDIR)
	@echo '[AS] $@'
	$(Q)$(AS) $(ASFLAGS) -c -o $@ $<

$(OUTDIR)/%.i: %.c | $(OUTDIR)
	@echo '[PP] $@'
	$(Q)$(CC) $(CFLAGS) -E -o $@ $<

$(OUTDIR)/$(TARGET).elf: $(OBJ)
	@echo '[LD] $@'
	$(Q)$(LD) $(OBJ) $(LDFLAGS) -o $@

$(OUTDIR)/%.dis: %.elf
	@echo '[OBJDUMP] $@'
	$(Q)$(OBJDUMP) -D $< > $@

$(OUTDIR):
	@mkdir -p $@

.PHONY: clean distclean

clean:
	@echo '[CLEAN]'
	rm -rf $(OUTDIR)

include $(wildcard $(DEPS))
