--- gdb/bsd-kvm.c.orig	2024-05-02 19:11:06 UTC
+++ gdb/bsd-kvm.c
@@ -41,7 +41,9 @@
 #include <paths.h>
 #include "readline/readline.h"
 #include <sys/param.h>
+#if !defined(__DragonFly__)
 #include <sys/proc.h>
+#endif
 #ifdef HAVE_SYS_USER_H
 #include <sys/user.h>
 #endif
@@ -328,7 +330,9 @@ bsd_kvm_proc_cmd (const char *arg, int f
 #ifdef HAVE_STRUCT_LWP
   addr += offsetof (struct lwp, l_addr);
 #else
+# ifndef __DragonFly__  /* XXX FIXME */
   addr += offsetof (struct proc, p_addr);
+# endif
 #endif
 
   if (kvm_read (core_kd, addr, &bsd_kvm_paddr, sizeof bsd_kvm_paddr) == -1)
