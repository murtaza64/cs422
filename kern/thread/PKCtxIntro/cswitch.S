/*
 * void cswitch(struct kctx *from, struct kctx *to);
 */

/*
struct kctx {
    void *esp;
    unsigned int edi;
    unsigned int esi;
    unsigned int ebx;
    unsigned int ebp;
    void *eip;
};
*/
	.globl cswitch
cswitch:
	movl	4(%esp), %eax	/* %eax <- from */
	movl	8(%esp), %edx	/* %edx <- to */

	/* TODO: save the old kernel context */
	movl	%esp, 0(%eax)
	movl	%edi, 4(%eax)
	movl	%esi, 8(%eax)
	movl	%ebx, 12(%eax)
	movl	%ebp, 16(%eax)
	movl	(%esp), %ebx
	movl	%ebx, 20(%eax)
	/* TODO: load the new kernel context */
	movl	0(%edx), %esp
	movl	4(%edx), %edi
	movl	8(%edx), %esi
	movl	12(%edx), %ebx
	movl	16(%edx), %ebp
	pushl	20(%edx)

	xor	%eax, %eax
	ret
