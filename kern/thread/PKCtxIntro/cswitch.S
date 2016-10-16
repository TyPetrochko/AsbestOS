/*
 * void cswitch(struct kctx *from, struct kctx *to);
 */

	.globl cswitch
cswitch:
	movl	  4(%esp), %eax	/* %eax <- from */
	movl	  8(%esp), %edx	/* %edx <- to */

	/* TODO: save the old kernel context */
  movl    %esp,  0(%eax)
  movl    %edi,  4(%eax)
  movl    %esi,  8(%eax)
  movl    %ebx, 12(%eax)
  movl    %ebp, 16(%eax)
  pushl   (%esp)
  popl    20(%eax)

	/* TODO: load the new kernel context */

  // Pseudocode:
  //  0) Save ESP
  //  1) Set actual edi to struct edi
  //  2) Set actual esi to struct esi
  //  3) Same for ebx, ebp...
  //  4) Set 0(%esp) to struct eip
  
  movl     0(%edx), %esp 
  movl     4(%edx), %edi 
  movl     8(%edx), %esi
  movl    12(%edx), %ebx 
  movl    16(%edx), %ebp 
  pushl   20(%edx)
  popl    (%esp)
	xor     %eax, %eax
	ret
