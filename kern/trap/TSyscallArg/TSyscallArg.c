#include <lib/trap.h>
#include <lib/x86.h>

#include "import.h"

unsigned int syscall_get_arg1(tf_t *tf)
{
  return tf -> regs.eax;
}

unsigned int syscall_get_arg2(tf_t *tf)
{
  return tf -> regs.ebx;
}

unsigned int syscall_get_arg3(tf_t *tf)
{
  return tf -> regs.ecx;
}

unsigned int syscall_get_arg4(tf_t *tf)
{
  return tf -> regs.edx;
}

unsigned int syscall_get_arg5(tf_t *tf)
{
  return tf -> regs.esi;
}

unsigned int syscall_get_arg6(tf_t *tf)
{
  return tf -> regs.edi;
}

void syscall_set_errno(tf_t *tf, unsigned int errno)
{
  tf -> regs.eax = errno;
}

void syscall_set_retval1(tf_t *tf, unsigned int retval)
{
  tf -> regs.ebx = retval;
}

void syscall_set_retval2(tf_t *tf, unsigned int retval)
{
  tf -> regs.ecx = retval;
}

void syscall_set_retval3(tf_t *tf, unsigned int retval)
{
  tf -> regs.edx = retval;
}

void syscall_set_retval4(tf_t *tf, unsigned int retval)
{
  tf -> regs.esi = retval;
}

void syscall_set_retval5(tf_t *tf, unsigned int retval)
{
  tf -> regs.edi = retval;
}
