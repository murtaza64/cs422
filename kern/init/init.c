#include <lib/debug.h>
#include <lib/types.h>
#include <lib/kstack.h>
#include <lib/thread.h>
#include <lib/x86.h>
#include <dev/devinit.h>
#include <pcpu/PCPUIntro/export.h>
#include <proc/PProc/export.h>
#include <thread/PCurID/export.h>
#include <thread/PKCtxIntro/export.h>
#include <thread/PTCBIntro/export.h>
#include <thread/PTQueueInit/export.h>
#include <thread/PThread/export.h>

extern uint32_t pcpu_ncpu(void);

static volatile int cpu_booted = 0;
static volatile int all_ready = FALSE;

extern int pcpu_boot_ap(uint32_t cpu_idx, void (*f)(void), uintptr_t stack_addr);
static void kern_main_ap(void);

extern spinlock_t sched_lk;

extern uint8_t _binary___obj_user_idle_idle_start[];
extern uint8_t _binary___obj_user_pingpong_ping_start[];
extern uint8_t _binary___obj_user_pingpong_pong_start[];

static void kern_main(void)
{
    KERN_INFO("[BSP KERN] In kernel main.\n\n");

    KERN_INFO("[BSP KERN] Number of CPUs in this system: %d. \n", pcpu_ncpu());

    int cpu_idx = get_pcpu_idx();
    unsigned int pid;

    for (int i = 1; i < pcpu_ncpu(); i++) {
        KERN_INFO("[BSP KERN] Boot CPU %d .... \n", i);

        bsp_kstack[i].cpu_idx = i;
        pcpu_boot_ap(i, kern_main_ap, (uintptr_t) &bsp_kstack[i]);

        while (get_pcpu_boot_info(i) == FALSE);

        KERN_INFO("[BSP KERN] done.\n");
    }

    all_ready = TRUE;

    // pid = proc_create(_binary___obj_user_idle_idle_start, 10000);
    // KERN_INFO("CPU%d: process idle %d is created.\n", cpu_idx, pid);
    // tqueue_remove(NUM_IDS, pid);
    // tcb_set_state(pid, TSTATE_RUN);
    // set_curid(pid);
    // kctx_switch(0, pid);

    // KERN_PANIC("kern_main() should never reach here.\n");
}

static void kern_main_ap(void)
{
    int cpu_idx = get_pcpu_idx();

    set_pcpu_boot_info(cpu_idx, TRUE);

    while (all_ready == FALSE);

    KERN_INFO("[AP%d KERN] kernel_main_ap\n", cpu_idx);

    cpu_booted++;
    spinlock_acquire(&sched_lk);
    KERN_INFO("[AP KERN] sched lock acquired\n");
    unsigned int pid, pid2, pid3, pid4, pid5, pid6;
    if (cpu_idx == 1) {
        pid = proc_create(_binary___obj_user_pingpong_ping_start, 1000);
        KERN_INFO("CPU%d: process ping1 %d is created.\n", cpu_idx, pid);
        pid2 = proc_create(_binary___obj_user_pingpong_ping_start, 1000);
        KERN_INFO("CPU%d: process ping2 %d is created.\n", cpu_idx, pid2);
        pid4 = proc_create(_binary___obj_user_pingpong_ping_start, 1000);
        KERN_INFO("CPU%d: process pong3 %d is created.\n", cpu_idx, pid4);
        pid5 = proc_create(_binary___obj_user_pingpong_ping_start, 1000);
        KERN_INFO("CPU%d: process pong4 %d is created.\n", cpu_idx, pid5);
        pid6 = proc_create(_binary___obj_user_pingpong_ping_start, 1000);
        KERN_INFO("CPU%d: process pong5 %d is created.\n", cpu_idx, pid6);
        
        pid3 = proc_create(_binary___obj_user_idle_idle_start, 1000);
        KERN_INFO("CPU%d: process idle1 %d is created.\n", cpu_idx, pid3);
    }
    else if (cpu_idx == 2) {
        pid = proc_create(_binary___obj_user_pingpong_pong_start, 1000);
        KERN_INFO("CPU%d: process pong1 %d is created.\n", cpu_idx, pid);
        pid2 = proc_create(_binary___obj_user_pingpong_pong_start, 1000);
        KERN_INFO("CPU%d: process pong2 %d is created.\n", cpu_idx, pid2);
        pid4 = proc_create(_binary___obj_user_pingpong_ping_start, 1000);
        KERN_INFO("CPU%d: process ping3 %d is created.\n", cpu_idx, pid4);
        pid5 = proc_create(_binary___obj_user_pingpong_ping_start, 1000);
        KERN_INFO("CPU%d: process ping4 %d is created.\n", cpu_idx, pid5);

        pid3 = proc_create(_binary___obj_user_idle_idle_start, 1000);
        KERN_INFO("CPU%d: process idle2 %d is created.\n", cpu_idx, pid3);
    }
    else {
        spinlock_release(&sched_lk);
        return;
    }

    tqueue_remove(NUM_IDS + cpu_idx, pid);
    tcb_set_state(pid, TSTATE_RUN);
    set_curid(pid);
    // KERN_INFO("CPU%d context switching to %d\n", cpu_idx, pid);
    // KERN_INFO("CPU%d curid=%d\n", cpu_idx, get_curid());
    spinlock_release(&sched_lk);
    kctx_switch(0, pid);

    KERN_PANIC("kern_main_ap() should never reach here.\n");
}

void kern_init(uintptr_t mbi_addr)
{
    thread_init(mbi_addr);
    KERN_INFO("[BSP KERN] Kernel initialized.\n");
    kern_main();
}

void kern_init_ap(void (*f)(void))
{
    devinit_ap();
    f();
}
