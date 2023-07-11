--- gdb/bsd-kvm.c.orig	2023-07-11 00:50:35 UTC
+++ gdb/bsd-kvm.c
@@ -328,7 +328,9 @@ bsd_kvm_proc_cmd (const char *arg, int f
 #ifdef HAVE_STRUCT_LWP
   addr += offsetof (struct lwp, l_addr);
 #else
+# ifndef __DragonFly__  /* XXX FIXME */
   addr += offsetof (struct proc, p_addr);
+# endif
 #endif
 
   if (kvm_read (core_kd, addr, &bsd_kvm_paddr, sizeof bsd_kvm_paddr) == -1)
