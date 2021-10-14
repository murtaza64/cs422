#include <lib/elf.h>
#include <lib/debug.h>
#include <lib/gcc.h>
#include <lib/seg.h>
#include <lib/trap.h>
#include <lib/x86.h>

#include "import.h"

extern tf_t uctx_pool[NUM_IDS];
extern char STACK_LOC[NUM_IDS][PAGESIZE];

void proc_start_user(void)
{
    unsigned int cur_pid = get_curid();
    tss_switch(cur_pid);
    set_pdir_base(cur_pid);

    trap_return((void *) &uctx_pool[cur_pid]);
}

unsigned int proc_create(void *elf_addr, unsigned int quota)
{
    unsigned int pid, id;

    id = get_curid();
    pid = thread_spawn((void *) proc_start_user, id, quota);

    if (pid != NUM_IDS) {
        elf_load(elf_addr, pid);

        uctx_pool[pid].es = CPU_GDT_UDATA | 3;
        uctx_pool[pid].ds = CPU_GDT_UDATA | 3;
        uctx_pool[pid].cs = CPU_GDT_UCODE | 3;
        uctx_pool[pid].ss = CPU_GDT_UDATA | 3;
        uctx_pool[pid].esp = VM_USERHI;
        uctx_pool[pid].eflags = FL_IF;
        uctx_pool[pid].eip = elf_entry(elf_addr);
    }

    return pid;
}
//creates a new thread for forked process and return its id (or NUMIDS). sets eax for
//forked process to 0.
unsigned int proc_fork()
{
    unsigned int pid, forked_id;

    pid = get_curid();
    unsigned int quota = container_get_quota(pid);
    unsigned int usage = container_get_usage(pid);
    KERN_DEBUG("parent has quota=%d usage=%d\n", quota, usage);
    unsigned int avail = (quota - usage)/2;
    if (avail == 0) {
        return NUM_IDS;
    }
    forked_id = thread_spawn((void *) proc_start_user, pid, avail);

    if (forked_id != NUM_IDS) {
        uctx_pool[forked_id] = uctx_pool[pid];
        uctx_pool[forked_id].regs.eax = 0;
        uctx_pool[forked_id].regs.ebx = 0; //HACK remove??
    }
    KERN_DEBUG("parent is pid %d\n", pid);
    KERN_DEBUG("child is pid %d\n", forked_id);
    KERN_DEBUG("child eax=%d\n", uctx_pool[forked_id].regs.eax);

    return forked_id;
}