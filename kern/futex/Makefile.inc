# -*-Makefile-*-

OBJDIRS += $(KERN_OBJDIR)/futex

KERN_SRCFILES += $(KERN_DIR)/futex/futex_map.c
KERN_SRCFILES += $(KERN_DIR)/futex/futex_queue.c
KERN_SRCFILES += $(KERN_DIR)/futex/futex.c
KERN_SRCFILES += $(KERN_DIR)/futex/sysfutex.c

$(KERN_OBJDIR)/futex/%.o: $(KERN_DIR)/futex/%.c
	@echo + cc[KERN/futex] $<
	@mkdir -p $(@D)
	$(V)$(CC) $(KERN_CFLAGS) -c -o $@ $<

$(KERN_OBJDIR)/futex/%.o: $(KERN_DIR)/futex/%.S
	@echo + as[KERN/futex] $<
	@mkdir -p $(@D)
	$(V)$(CC) $(KERN_CFLAGS) -c -o $@ $<
