--- gdb/i386-bsd-nat.c.orig
+++ gdb/i386-bsd-nat.c
@@ -247,6 +247,8 @@ INIT_GDB_FILE (i386bsd_nat)
 
 #if defined (OpenBSD)
 #define SC_REG_OFFSET i386obsd_sc_reg_offset
+#elif defined (DragonFly)
+#define SC_REG_OFFSET i386dfly_sc_reg_offset
 #endif
 
 #ifdef SC_REG_OFFSET
