#ifndef _SYS_VIRT_PIC_H_
#define _SYS_VIRT_PIC_H_

#ifdef _KERN_

#include <sys/types.h>

#include <sys/virt/vmm.h>

struct i8259 {
	struct vpic	*vpic;

	bool		master;

	bool		single_mode;

	uint8_t		init_state;
	bool		init4;

	uint8_t		lowest_priority;

	uint8_t		irq_base;
	uint8_t		last_irr;
	uint8_t		irr;
	uint8_t		isr;
	uint8_t		imr;
	uint8_t		int_out;

	bool		select_isr;
	bool		poll;
	bool		auto_eoi_mode;
	bool		rotate_on_auto_eoi;
	bool		special_mask_mode;
	bool 		fully_nested_mode;
	bool		special_fully_nested_mode;

	uint8_t		elcr;
	uint8_t		elcr_mask;

	bool		ready;
};

struct vpic {
	uint8_t		int_out;
	struct i8259	master;
	struct i8259	slave;
};

void vpic_init(struct vpic *, struct vm *);
bool vpic_has_irq(struct vpic *);
int vpic_read_irq(struct vpic *);
void vpic_set_irq(struct vpic *, int irq, int level);
bool vpic_is_ready(struct vpic *);

#endif /* _KERN_ */

#endif /* !_SYS_VIRT_PIC_H_ */