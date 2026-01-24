--- gdb/i386-fbsd-nat.c.orig
+++ gdb/i386-fbsd-nat.c
@@ -39,8 +39,6 @@ public:
   void store_registers (struct regcache *, int) override;
 
   const struct target_desc *read_description () override;
-
-  void resume (ptid_t, int, enum gdb_signal) override;
 };
 
 static i386_fbsd_nat_target the_i386_fbsd_nat_target;
@@ -215,6 +213,7 @@ i386_fbsd_nat_target::store_registers (s
     perror_with_name (_("Couldn't write floating point status"));
 }
 
+#if 0
 /* Resume execution of the inferior process.  If STEP is nonzero,
    single-step it.  If SIGNAL is nonzero, give it that signal.  */
 
@@ -261,6 +260,7 @@ i386_fbsd_nat_target::resume (ptid_t pti
 	      gdb_signal_to_host (signal)) == -1)
     perror_with_name (("ptrace"));
 }
+#endif
 
 
 /* Support for debugging kernel virtual memory images.  */
