/*
 * QEMU PC keyboard emulation
 *
 * Copyright (c) 2003 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/*
 * Adapted for CertiKOS by Haozhong Zhang at Yale University.
 */

#ifndef _SYS_VIRT_DEV_KBD_H_
#define _SYS_VIRT_DEV_KBD_H_

#ifdef _KERN_

#include <sys/types.h>

#include <sys/virt/vmm.h>
#include <sys/virt/dev/ps2.h>

struct vkbd {
	struct vm	*vm;

	uint8_t		write_cmd;

	uint8_t		status;		/* buffer of command/status port */
	uint8_t		outport;	/* buffer of data port */

	uint8_t		mode;

	uint8_t		pending;	/* KBD/AUX event pending? */

	struct PS2_kbd		kbd;
	struct PS2_mouse	mouse;
};

void vkbd_init(struct vkbd *, struct vm *);

#endif /* _KERN_ */

#endif /* !_SYS_VIRT_DEV_KBD_H_ */
