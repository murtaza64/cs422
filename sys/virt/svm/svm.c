#include <sys/debug.h>
#include <sys/intr.h>
#include <sys/mem.h>
#include <sys/string.h>
#include <sys/types.h>
#include <sys/x86.h>

#include <sys/virt/vmm.h>
#include <sys/virt/vmm_dev.h>
#include <sys/virt/dev/kbd.h>
#include <sys/virt/dev/pci.h>
#include <sys/virt/dev/pic.h>

#include <machine/pcpu.h>
#include <machine/pmap.h>

#include <dev/kbd.h>
#include <dev/lapic.h>
#include <dev/pic.h>

#include "svm.h"
#include "svm_handle.h"
#include "svm_utils.h"

/* struct vcpu { */
/* 	uint64_t	flags; */
/* 	int 		h_cpu; */
/* 	uint64_t	g_msrs[SVM_MSR_NUM]; */
/* 	struct vlapic	*vlapic; */
/* 	int		vcpuid; */
/* }; */

struct {
	struct svm 	svms[4];
	bool		used[4];
} svm_info;

/*
 * data allocator.
 */
static struct svm *
alloc_svm(void)
{
	int i;

	for (i = 0; i < 4; i++)
		if (svm_info.used[i] == FALSE)
			break;

	if (i == 4)
		return NULL;
	else {
		memset(&svm_info.svms[i], 0x0, sizeof(struct svm));
		svm_info.used[i] = TRUE;
		return &svm_info.svms[i];
	}
}

/*
 * Check whether the machine supports SVM. (Sec 15.4, APM Vol2 r3.19)
 *
 * @return TRUE if SVM is supported; otherwise FALSE
 */
static bool
svm_check(void)
{
	/* check CPUID 0x80000001 */
	uint32_t feature, dummy;
	cpuid(CPUID_FEATURE_FUNC, &dummy, &dummy, &feature, &dummy);
	if ((feature & CPUID_X_FEATURE_SVM) == 0) {
		/* KERN_DEBUG("The processor does not support SVM.\n"); */
		return FALSE;
	}

	/* check MSR VM_CR */
	if ((rdmsr(MSR_VM_CR) & MSR_VM_CR_SVMDIS) == 0) {
		return TRUE;
	}

	/* check CPUID 0x8000000a */
	cpuid(CPUID_SVM_FEATURE_FUNC, &dummy, &dummy, &dummy, &feature);
	if ((feature & CPUID_SVM_LOCKED) == 0) {
		/* KERN_DEBUG("SVM maybe disabled by BIOS.\n"); */
		return FALSE;
	} else {
		/* KERN_DEBUG("SVM maybe disabled with key.\n"); */
		return FALSE;
	}

	/* KERN_DEBUG("SVM is available.\n"); */

	return TRUE;
}

/*
 * Enable SVM. (Sec 15.4, APM Vol2 r3.19)
 */
static void
svm_enable(void)
{
	/* set MSR_EFER.SVME */
	uint64_t efer;

	efer = rdmsr(MSR_EFER);
	efer |= MSR_EFER_SVME;
	wrmsr(MSR_EFER, efer);

	/* KERN_DEBUG("SVM is enabled.\n"); */
}

/*
 * Allocate one memory page for the host state-save area.
 */
static uintptr_t
alloc_hsave_area(void)
{
	pageinfo_t *pi = mem_page_alloc();

	if (pi == NULL) {
		KERN_DEBUG("Failed to allocate memory for the host state-save area failed.\n");
		return 0x0;
	}

	return mem_pi2phys(pi);
}


/*
 * Allocate one memory page for VMCB.
 *
 * @return the pointer to VMCB if succeed; otherwise NULL.
 */
static struct vmcb *
alloc_vmcb(void)
{
	pageinfo_t *pi = mem_page_alloc();

	if (pi == NULL) {
		KERN_DEBUG("Failed to allocate memory for VMCB.\n");
		return NULL;
	}

	memset((struct vmcb *) mem_pi2phys(pi), 0x0, sizeof(struct vmcb));

	return (struct vmcb *) mem_pi2phys(pi);
}

/*
 * Create the nestd page table.
 * - identically maps the lowest 1MB guest physical memory
 * - identically maps the memory hole above 0xf0000000
 *
 * @return the pointer to the page table if succeed; otherwise NULL.
 */
static pmap_t *
alloc_nested_ptable(void)
{
	uintptr_t addr;
	pmap_t *pmap = pmap_new_empty();

	if (pmap == NULL) {
		KERN_DEBUG("Failed to allocate memory for nested page table.\n");
		return NULL;
	}

	for (addr = 0x0; addr < VM_PHY_MEMORY_SIZE; addr += PAGESIZE) {
		if (addr >= 0xa0000 && addr <= 0xbffff) {
			/* identically map VGA display memory to the host */
			pmap_insert(pmap,
				    mem_phys2pi(addr), addr, PTE_G | PTE_W | PTE_U);
		} else if (pmap_reserve(pmap, addr,
					PTE_G | PTE_W | PTE_U) == NULL) {
			KERN_DEBUG("Failed to map guest memory page at %x.\n",
				   addr);
			pmap_free(pmap);
			return NULL;
		}
	}

	return pmap;
}

/*
 * Create an permission map. The permission map is aligned to the
 * boundary of the physical memory page and occupies continuous
 * ROUNDUP(size, PAGESIZE) bytes of physical memory.
 *
 * @param size the least size of the permission map in bytes
 *
 * @return the 64-bit address of the permission map if succeed;
 *         otherwise, 0x0.
 */
static uint64_t
alloc_permission_map(size_t size)
{
	pageinfo_t *pi = mem_pages_alloc(size);

	if (pi == NULL)
		return 0;

	uintptr_t addr = mem_pi2phys(pi);
	memset((uint8_t *) addr, 0, size);

	return (uint64_t) addr;
}

void
set_intercept_ioio(struct vmcb *vmcb, uint32_t port, bool enable)
{
	KERN_ASSERT(vmcb != NULL);

	uint32_t *iopm = (uint32_t *)(uintptr_t) vmcb->control.iopm_base_pa;

	int entry = port / 32;
	int bit = port - entry * 32;

	if (enable)
		iopm[entry] |= (1 << bit);
	else
		iopm[entry] &= ~(1 << bit);
}

void
set_intercept_rdmsr(struct vmcb *vmcb, uint64_t msr, bool enable)
{
	KERN_PANIC("Not implemented yet.\n");
}

void
set_intercept_wrmsr(struct vmcb *vmcb, uint64_t msr, bool enable)
{
	KERN_PANIC("Not implemented yet.\n");
}

/*
 * Set an interception bit in VMCB.
 *
 * @param vmcb the pointer to a VMCB
 * @param bit the interception bit to be set
 */
void
set_intercept(struct vmcb *vmcb, int bit, bool enable)
{
	KERN_ASSERT(vmcb != NULL);
	KERN_ASSERT(INTERCEPT_INTR <= bit && bit <= INTERCEPT_XSETBV);

	if (enable == TRUE)
		vmcb->control.intercept |= (1ULL << bit);
	else
		vmcb->control.intercept &= ~(1ULL << bit);
}

void
set_intercept_exception(struct vmcb *vmcb, int bit, bool enable)
{
	KERN_ASSERT(vmcb != NULL);
	KERN_ASSERT(0 <= bit && bit < 32);

	if (enable == TRUE)
		vmcb->control.intercept_exceptions |= (1UL << bit);
	else
		vmcb->control.intercept_exceptions &= ~(1UL << bit);
}

/*
 * Setup the interception.
 *
 * @param vmcb the pointer to a VMCB
 *
 * @return 0 if succeed; otherwise, 1.
 */
static int
setup_intercept(struct vm *vm)
{
	KERN_ASSERT(vm != NULL);

	int i;

	struct svm *svm = (struct svm *) vm->cookie;
	struct vmcb *vmcb = svm->vmcb;

	/* create IOPM */
	vmcb->control.iopm_base_pa = alloc_permission_map(SVM_IOPM_SIZE);

	if (vmcb->control.iopm_base_pa == 0x0) {
		KERN_DEBUG("Failed to create IOPM.\n");
		return 1;
	}

	KERN_DEBUG("IOPM is at %x.\n", vmcb->control.iopm_base_pa);

	/* setup IOIO intercept */
	for (i = 0 ; i < MAX_IOPORT; i++)
		if (vm->iodev[i].dev != NULL) {
			set_intercept_ioio(vmcb, i, TRUE);
		}

	/* create MSRPM */
	vmcb->control.msrpm_base_pa = alloc_permission_map(SVM_MSRPM_SIZE);

	if (vmcb->control.msrpm_base_pa == 0x0) {
		KERN_DEBUG("Failed to create MSRPM.\n");
		return 1;
	}

	KERN_DEBUG("MSRPM is at %x.\n", vmcb->control.msrpm_base_pa);

	/* enable interceptions */
	set_intercept(vmcb, INTERCEPT_VMRUN, TRUE);
	set_intercept(vmcb, INTERCEPT_VMMCALL, TRUE);
	set_intercept(vmcb, INTERCEPT_STGI, TRUE);
	set_intercept(vmcb, INTERCEPT_CLGI, TRUE);
	set_intercept(vmcb, INTERCEPT_INTR, TRUE);
	set_intercept(vmcb, INTERCEPT_IOIO_PROT, TRUE);
	set_intercept(vmcb, INTERCEPT_CPUID, TRUE);
	/* set_intercept(vmcb, INTERCEPT_INTn, TRUE); */
	set_intercept(vmcb, INTERCEPT_VINTR, TRUE);
	/* set_intercept(vmcb, INTERCEPT_RDTSC, TRUE); */

	/* setup exception intercept */
	set_intercept_exception(vmcb, T_DEBUG, TRUE);

	return 0;
}

static void
set_segment(struct vmcb_seg *seg,
	    uint16_t sel, uint64_t base, uint32_t lim, uint16_t attr)
{
	KERN_ASSERT(seg != NULL);

	seg->selector = sel;
	seg->base = base;
	seg->limit = lim;
	seg->attrib = attr;
}

/*
 * Setup VM to the powerup state. (Sec 14.1.3, APM Vol2 r3.19)
 */
static void
setup_powerup_state(struct vm *vm)
{
	KERN_ASSERT(vm != NULL);

	struct svm *svm = (struct svm *) vm->cookie;
	struct vmcb_save_area *save = &svm->vmcb->save;

	save->cr0 = 0x60000010;
	save->cr2 = save->cr3 = save->cr4 = 0;
	save->rflags = 0x00000002;
	save->efer = MSR_EFER_SVME;
	save->rip = 0x0000fff0;
	set_segment(&save->cs, 0xf000, 0xffff0000, 0xffff,
		    SEG_ATTR_P | SEG_ATTR_S | SEG_TYPE_CODE);
	set_segment(&save->ds, 0x0000, 0x0, 0xffff,
		    SEG_ATTR_P | SEG_ATTR_S | SEG_TYPE_DATA);
	set_segment(&save->es, 0x0000, 0x0, 0xffff,
		    SEG_ATTR_P | SEG_ATTR_S | SEG_TYPE_DATA);
	set_segment(&save->fs, 0x0000, 0x0, 0xffff,
		    SEG_ATTR_P | SEG_ATTR_S | SEG_TYPE_DATA);
	set_segment(&save->gs, 0x0000, 0x0, 0xffff,
		    SEG_ATTR_P | SEG_ATTR_S | SEG_TYPE_DATA);
	set_segment(&save->ss, 0x0000, 0x0, 0xffff,
		    SEG_ATTR_P | SEG_ATTR_S | SEG_TYPE_DATA);
	set_segment(&save->gdtr, 0, 0x0, 0xffff, 0);
	set_segment(&save->idtr, 0, 0x0, 0xffff, 0);
	set_segment(&save->ldtr, 0, 0x0, 0xffff, SEG_ATTR_P | SEG_TYPE_LDT);
	set_segment(&save->tr, 0, 0x0, 0xffff, SEG_ATTR_P | SEG_TYPE_TSS_BUSY);
	save->rax = 0x0;
	save->dr6 = 0xffff0ff0;
	save->dr7 = 0x00000400;
	save->g_pat = 0x7040600070406ULL;

	svm->g_rbx = svm->g_rcx = svm->g_rsi = svm->g_rdi = svm->g_rbp = 0x0;
	svm->g_rdx = 0x80;
}

/*
 * Initialize SVM module.
 * - check whether SVM is supported
 * - enable SVM
 * - allocate host save area
 *
 * @return 0 if succeed
 */
static int
svm_init(void)
{
	uintptr_t hsave_addr;

	/* check whether the processor supports SVM */
	if (svm_check() == FALSE)
		return 1;

	/* enable SVM */
	svm_enable();

	/* setup host state-save area */
	if ((hsave_addr = alloc_hsave_area()) == 0x0)
		return 1;
	else
		wrmsr(MSR_VM_HSAVE_PA, hsave_addr);

	/* KERN_DEBUG("Host state-save area is at %x.\n", hsave_addr); */

	/* register guest interrupt handler  */
	trap_register_default_handler(svm_guest_intr_handler);
	trap_register_kern_handler(T_GPFLT, svm_guest_handle_gpf);

	return 0;
}

/*
 * Initialize a VM instant.
 */
static int
vm_init(struct vm *vm)
{
	KERN_ASSERT(vm != NULL);

	struct svm *svm = alloc_svm();

	if (svm == NULL)
		return 1;

	vm->cookie = svm;

	/* allocate memory for VMCB */
	if ((svm->vmcb = alloc_vmcb()) == NULL)
	    return 1;

	KERN_DEBUG("VMCB is at %0x.\n", svm->vmcb);

	/* setup VM to the powerup state */
	setup_powerup_state(vm);

	/* create the nested page table */
	pmap_t *ncr3 = alloc_nested_ptable();

	if (ncr3 == NULL)
		return 1;

	svm->vmcb->control.nested_cr3 = (uintptr_t) ncr3;

	KERN_DEBUG("Nested page table is at %x.\n", ncr3);

	/* load seabios */
	load_bios((uintptr_t) ncr3);

	/* setup interception */
	if (setup_intercept(vm) != 0)
		return 1;

	/* miscellenea initialization */
	svm->vmcb->control.asid = 1;
	svm->vmcb->control.nested_ctl = 1;
	svm->vmcb->control.tlb_ctl = 0;
	svm->vmcb->control.tsc_offset = 0;
	/* Sec 15.21.1, APM Vol2 r3.19 */
	svm->vmcb->control.int_ctl = (SVM_INTR_CTRL_VINTR_MASK |
				      (0x0 & SVM_INTR_CTRL_VTPR));

	return 0;
}

/*
 * Start the VM.
 */
static int
vm_run(struct vm *vm)
{
	KERN_ASSERT(vm != NULL);

	struct svm *svm = (struct svm *) vm->cookie;

	SVM_CLGI();

	sti();
	svm_run(svm);
	cli();

	if(svm->vmcb->control.exit_code == SVM_EXIT_INTR)
		vm->exit_for_intr = TRUE;

	SVM_STGI();

	return 0;
}

static int
svm_handle_exit(struct vm *vm)
{
	KERN_ASSERT(vm != NULL);

	struct svm *svm = (struct svm *) vm->cookie;
	struct vmcb *vmcb = svm->vmcb;
	struct vmcb_control_area *ctrl = &vmcb->control;

	bool handled = FALSE;

	KERN_DEBUG("[%x:%llx] ",
		   svm->vmcb->save.cs.selector, svm->vmcb->save.rip);

	switch (ctrl->exit_code) {
	case SVM_EXIT_EXCP_BASE ... (SVM_EXIT_INTR-1):
		dprintf("VMEXIT for EXCP ");
		handled = svm_handle_exception(vm);
		break;

	case SVM_EXIT_INTR:
		/* kernel interrupt handlers should come before here */
		dprintf("VMEXIT for INTR.\n");
		handled = svm_handle_intr(vm);
		break;

	case SVM_EXIT_VINTR:
		dprintf("VMEXIT for VINTR.\n");
		handled = svm_handle_vintr(vm);
		break;

	case SVM_EXIT_IOIO:
		dprintf("VMEXIT for IO");
		handled = svm_handle_ioio(vm);
		break;

	case SVM_EXIT_NPF:
		dprintf("VMEXIT for NPF");
		handled = svm_handle_npf(vm);
		break;

	case SVM_EXIT_CPUID:
		dprintf("VMEXIT for cpuid");
		handled = svm_handle_cpuid(vm);
		break;

	case SVM_EXIT_SWINT:
		dprintf("VMEXIT for INTn.\n");
		handled = svm_handle_swint(vm);
		break;

	case SVM_EXIT_RDTSC:
		dprintf("VMEXIT for RDTSC.\n");
		handled = svm_handle_rdtsc(vm);
		break;

	case SVM_EXIT_ERR:
		dprintf("VMEXIT for invalid guest state in VMCB.\n");
		handled = svm_handle_err(vm);
		break;

	default:
		KERN_PANIC("Unhandled VMEXIT: exit_code = %x.\n",
			   ctrl->exit_code);
	}

	if (ctrl->exit_code == SVM_EXIT_ERR)
		KERN_PANIC("Halt for SVM_EXIT_ERR.\n");

	if (handled == FALSE) {
		KERN_PANIC("Unhandled VMEXIT: exit_code = %x.\n",
			   ctrl->exit_code);
	}

	if (vpic_has_irq(&vm->vpic) == TRUE) {
		int irq = vpic_read_irq(&vm->vpic);
		svm_inject_event(vmcb, SVM_EVTINJ_TYPE_INTR, irq, FALSE, 0);
	}

	return 0;
}

struct vmm_ops vmm_ops_amd = {
	.vmm_init	= svm_init,
	.vm_init	= vm_init,
	.vm_run		= vm_run,
	.vm_handle	= svm_handle_exit
};