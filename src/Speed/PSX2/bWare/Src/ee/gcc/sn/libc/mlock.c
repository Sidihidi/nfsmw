/* __malloc_lock y __malloc_unlock de la libc de SN para PS2 (libc.a(mlock.obj)): un cerrojo recursivo sobre el
   semaforo __sce_sema_id del kernel, con el hilo dueno y la cuenta de entradas. */

#include <reent.h>
#include <eekernel.h>

extern int __sce_sema_id;

static int lock_owner = -1;
static int lock_count = 0;

void
__malloc_lock (struct _reent *ptr)
{
  int id = GetThreadId ();

  if (lock_owner == id)
    {
      lock_count++;
      return;
    }
  WaitSema (__sce_sema_id);
  lock_owner = id;
  lock_count++;
}

void
__malloc_unlock (struct _reent *ptr)
{
  if (--lock_count == 0)
    {
      lock_owner = -1;
      SignalSema (__sce_sema_id);
    }
}
