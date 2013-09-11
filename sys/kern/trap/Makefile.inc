# -*-Makefile-*-

OBJDIRS		+= $(KERN_OBJDIR)/kern/trap

KERN_SRCFILES	+= $(KERN_DIR)/kern/trap/trap.c
KERN_SRCFILES	+= $(KERN_DIR)/kern/trap/interrupt.c
KERN_SRCFILES	+= $(KERN_DIR)/kern/trap/exception.c
KERN_SRCFILES	+= $(KERN_DIR)/kern/trap/syscall.c

$(KERN_OBJDIR)/kern/trap/%.o: $(KERN_DIR)/kern/trap/%.c
	@echo + cc[KERN/kern] $<
	@mkdir -p $(@D)
	$(V)$(CC) $(KERN_CFLAGS) -c -o $@ $<

$(KERN_OBJDIR)/kern/trap/%.o: $(KERN_DIR)/kern/trap/%.S
	@echo + as[KERN/kern] $<
	@mkdir -p $(@D)
	$(V)$(CC) $(KERN_CFLAGS) -c -o $@ $<