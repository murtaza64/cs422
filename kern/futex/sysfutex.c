#include "futex_map.h"
#include "futex_queue.h"

#include <kern/lib/trap.h>
#include <kern/lib/syscall.h>
#include <kern/lib/types.h>
#include <kern/lib/thread.h>

#include <kern/trap/TSyscallArg/export.h>
#include <kern/vmm/MPTOp/export.h>
#include <kern/thread/PCurID/export.h>
#include <kern/thread/PThread/export.h>
#include <kern/thread/PTCBIntro/export.h>


//futex_wait(uaddr, val)
void sys_futex_wait(tf_t *tf) {
    uint32_t *uaddr, val, pid;
    uaddr = (uint32_t *) syscall_get_arg2(tf);
    val = syscall_get_arg3(tf);

    pid = get_curid();
    uint32_t *paddr = (uint32_t *) get_ptbl_entry_by_va(pid, (unsigned int) uaddr);

    if (*paddr != val) {
        syscall_set_retval1(tf, 1);
        syscall_set_errno(tf, E_AGAIN);
        return;
    }

    struct futexq *q = futex_map_get_or_create(paddr);

    spinlock_acquire(&q->lock);
    futexq_enqueue(q, get_curid());
    spinlock_release(&q->lock);

    thread_wait();
    syscall_set_errno(tf, E_SUCC);
    syscall_set_retval1(tf, 0);
}

//futex_wake(uaddr, n)
void sys_futex_wake(tf_t *tf) {
    uint32_t *uaddr, n, pid, n_woken, curid;
    uaddr = (uint32_t *) syscall_get_arg2(tf);
    n = syscall_get_arg3(tf);
    
    n_woken = 0;
    curid = get_curid();
    uint32_t *paddr = (uint32_t *) get_ptbl_entry_by_va(curid, (unsigned int) uaddr);

    struct futexq *q = futex_map_get_or_create(paddr);
    spinlock_acquire(&q->lock);
    while (n_woken < n && (pid = futexq_dequeue(q)) != NUM_IDS) {
        n_woken++;
        thread_ready(pid);
    }
    spinlock_release(&q->lock);
    syscall_set_errno(tf, E_SUCC);
    syscall_set_retval1(tf, n_woken);
    
}

// futex_cmp_requeue(uaddr, n_wake, n_move, uaddr2, val)
void sys_futex_cmp_requeue(tf_t *tf) {
    uint32_t *uaddr, n_wake, n_move, *uaddr2, val, n_woken, curid, n_moved, pid;
    uaddr = (uint32_t *) syscall_get_arg2(tf);
    n_wake = syscall_get_arg3(tf);
    n_move = syscall_get_arg4(tf);
    uaddr2 = (uint32_t *) syscall_get_arg5(tf);
    val = syscall_get_arg6(tf);

    n_woken = 0;
    n_moved = 0;
    curid = get_curid();
    uint32_t *paddr = (uint32_t *) get_ptbl_entry_by_va(curid, (unsigned int) uaddr);
    uint32_t *paddr2 = (uint32_t *) get_ptbl_entry_by_va(curid, (unsigned int) uaddr2);

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