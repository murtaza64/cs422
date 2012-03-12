# -*-Makefile-*-

ARCH_DIR	:= $(KERN_DIR)/arch/$(ARCH)
ARCH_OBJDIR	:= $(KERN_OBJDIR)/arch/$(ARCH)
ARCH_HEADERS	:= $(OBJDIR)/headers/machine

KERN_CFLAGS	+= -I$(OBJDIR)/headers

include		$(ARCH_DIR)/Makefile.inc

arch_headers:
	$(V)mkdir -p $(OBJDIR)/headers
	$(V)rm -f $(ARCH_HEADERS)
	$(V)ln -bs $(shell pwd)/$(ARCH_DIR)/include $(ARCH_HEADERS)