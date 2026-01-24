--- gdb/amd64-tdep.h.orig
+++ gdb/amd64-tdep.h
@@ -151,4 +151,9 @@ extern int amd64nbsd_r_reg_offset[];
 /* Variables exported from amd64-obsd-tdep.c.  */
 extern int amd64obsd_r_reg_offset[];
 
+/* Variables exported from amd64-dfly-tdep.c.  */
+extern CORE_ADDR amd64dfly_sigtramp_start_addr;
+extern CORE_ADDR amd64dfly_sigtramp_end_addr;
+extern int amd64dfly_sc_reg_offset[];
+
 #endif /* GDB_AMD64_TDEP_H */
