--- gdb/bsd-kvm.c.orig
+++ gdb/bsd-kvm.c
@@ -40,7 +40,9 @@
 #include <paths.h>
 #include "readline/readline.h"
 #include <sys/param.h>
+#if !defined(__DragonFly__)
 #include <sys/proc.h>
+#endif
 #ifdef HAVE_SYS_USER_H
 #include <sys/user.h>
 #endif
@@ -326,7 +328,9 @@ bsd_kvm_proc_cmd (const char *arg, int f
 #ifdef HAVE_STRUCT_LWP
   addr += offsetof (struct lwp, l_addr);
 #else
+# ifndef __DragonFly__  /* XXX FIXME */
   addr += offsetof (struct proc, p_addr);
+# endif
 #endif
 
   if (kvm_read (core_kd, addr, &bsd_kvm_paddr, sizeof bsd_kvm_paddr) == -1)
