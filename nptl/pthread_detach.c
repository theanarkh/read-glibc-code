/* Copyright (C) 2002-2024 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

#include <errno.h>
#include "pthreadP.h"
#include <atomic.h>
#include <shlib-compat.h>

int
___pthread_detach (pthread_t th)
{
  struct pthread *pd = (struct pthread *) th;

  /* Make sure the descriptor is valid.  */
  if (INVALID_NOT_TERMINATED_TD_P (pd))
    /* Not a valid thread handle.  */
    return ESRCH;

  int result = 0;

  /* Mark the thread as detached.  */
  /*
    atomic_compare_and_exchange_bool_acq 见 glibc/include/atomic.h
    &pd->joinid 的值为 NULL，则设置为 pd，表示处于 detached 状态，并返回 0
    &pd->joinid 的值不为 NULL，则不做任何修改直接返回 1，情况有两种
      1. pd->joinid 的值为 pd，表示已经处于 detached 状态
      2. pd->joinid 的值为其他 pthread_t，表示其他线程执行了 join pd，即等待 pd 退出，pd 不能执行 detach
  */
  if (atomic_compare_and_exchange_bool_acq (&pd->joinid, pd, NULL))
    {
      /* There are two possibilities here.  First, the thread might
	 already be detached.  In this case we return EINVAL.
	 Otherwise there might already be a waiter.  The standard does
	 not mention what happens in this case.  */
      if (IS_DETACHED (pd))
	result = EINVAL;
    }
  else
    /* Check whether the thread terminated meanwhile.  In this case we
       will just free the TCB.  */
    if ((pd->cancelhandling & EXITING_BITMASK) != 0)
      /* Note that the code in __free_tcb makes sure each thread
	 control block is freed only once.  */
      // 释放线程
      __nptl_free_tcb (pd);

  return result;
}
versioned_symbol (libc, ___pthread_detach, pthread_detach, GLIBC_2_34);
libc_hidden_ver (___pthread_detach, __pthread_detach)
#ifndef SHARED
strong_alias (___pthread_detach, __pthread_detach)
#endif

#if OTHER_SHLIB_COMPAT (libpthread, GLIBC_2_0, GLIBC_2_34)
compat_symbol (libc, ___pthread_detach, pthread_detach, GLIBC_2_0);
#endif
