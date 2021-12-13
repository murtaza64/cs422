#include "futex_map.h"
#include "futex_queue.h"

#include <kern/lib/trap.h>
#include <kern/lib/syscall.h>
#include <kern/lib/types.h>
#include <kern/lib/thread.h>
#include <kern/lib/debug.h>

#include <kern/trap/TSyscallArg/export.h>
#include <kern/vmm/MPTOp/export.h>
#include <kern/thread/PCurID/export.h>
#include <kern/thread/PThread/export.h>
#include <kern/thread/PTCBIntro/export.h>

#define UADDR_TO_PADDR(curid, uaddr) (uint32_t *) (~((1 << 12) - 1) & get_ptbl_entry_by_va(curid, (unsigned int) uaddr))


//futex_wait(uaddr, val)
void sys_futex_wait(tf_t *tf) {
    uint32_t *uaddr, val, pid;
    uaddr = (uint32_t *) syscall_get_arg2(tf);
    val = syscall_get_arg3(tf);

    pid = get_curid();
    KERN_INFO("[FUTEX WAIT] %d waiting on %x, val %d\n", pid, uaddr, val);
    uint32_t *paddr = UADDR_TO_PADDR(pid, uaddr);

    if (*paddr != val) {
        KERN_INFO("[FUTEX WAIT] val check failed, returning E_AGAIN\n");
        syscall_set_retval1(tf, 1);
        syscall_set_errno(tf, E_AGAIN);
        return;
    }
    // KERN_INFO("[FUTEX WAIT] %d waiting on %x\n", pid, uaddr);

    struct futexq *q = futex_map_get_or_create(paddr);

    spinlock_acquire(&q->lock);
    futexq_enqueue(q, get_curid());
    spinlock_release(&q->lock);

    thread_wait();
    KERN_INFO("[FUTEX WAIT] %d woken up on %x\n", pid, uaddr);
    syscall_set_errno(tf, E_SUCC);
    syscall_set_retval1(tf, 0);
}

//futex_wake(uaddr, n)
void sys_futex_wake(tf_t *tf) {
    uint32_t *uaddr, n, pid, n_woken, curid;
    uaddr = (uint32_t *) syscall_get_arg2(tf);
    n = syscall_get_arg3(tf);
    KERN_INFO("[FUTEX WAKE] waking up %d threads waiting on %x\n", n, uaddr);
    
    n_woken = 0;
    curid = get_curid();
    uint32_t *paddr = UADDR_TO_PADDR(curid, uaddr);
    // KERN_INFO("[FUTEX WAKE] paddr %x (unsigned int) uaddr %x entry %x\n", 
        // paddr, (unsigned int) uaddr, get_ptbl_entry_by_va(curid, (unsigned int) uaddr));
    struct futexq *q = futex_map_get_or_create(paddr);
    // KERN_INFO("[FUTEX WAKE] acquiring lock on futexq for %x...\n", uaddr);
    spinlock_acquire(&q->lock);
    while (n_woken < n && (pid = futexq_dequeue(q)) != NUM_IDS) {
        KERN_INFO("[FUTEX WAKE] woke up %d on %x\n", pid, uaddr);
        n_woken++;
        thread_ready(pid);
    }
    spinlock_release(&q->lock);
    syscall_set_errno(tf, E_SUCC);
    syscall_set_retval1(tf, n_woken);
    KERN_INFO("[FUTEX WAKE] woke up %d threads on %x\n", n_woken, uaddr);
    
}

// futex_cmp_requeue(uaddr, n_wake, n_move, uaddr2, val)
void sys_futex_cmp_requeue(tf_t *tf) {
    uint32_t *uaddr, n_wake, n_move, *uaddr2, val, n_woken, curid, n_moved, pid;
    uaddr = (uint32_t *) syscall_get_arg2(tf);
    n_wake = syscall_get_arg3(tf);
    n_move = syscall_get_arg4(tf);
    uaddr2 = (uint32_t *) syscall_get_arg5(tf);
    val = syscall_get_arg6(tf);
    KERN_INFO("[FUTEX CMP_REQUEUE] why is this running...?\n");
    n_woken = 0;
    n_moved = 0;
    curid = get_curid();
    uint32_t *paddr = UADDR_TO_PADDR(curid, uaddr);
    uint32_t *paddr2 = UADDR_TO_PADDR(curid, uaddr2);

    if (*paddr != val) {
        syscall_set_retval1(tf, 1);
        syscall_set_errno(tf, E_AGAIN);
        return;
    }

    struct futexq *q = futex_map_get_or_create(paddr);
    struct futexq *q2 = futex_map_get_or_create(paddr2);
    
    // Awake n_wake threads
    spinlock_acquire(&q->lock);
    while (n_woken < n_wake && (pid = futexq_dequeue(q)) != NUM_IDS) {
        n_woken++;
        thread_ready(pid);
    }
    spinlock_release(&q->lock);

    // Requeue up to n_move threads
    spinlock_acquire(&q2->lock);
    while (n_moved < n_move && (pid = futexq_dequeue(q)) != NUM_IDS) {
        futexq_enqueue(q2, pid);
    }
    spinlock_release(&q2->lock);

    syscall_set_errno(tf, E_SUCC);
    syscall_set_retval1(tf, n_woken + n_moved);
}