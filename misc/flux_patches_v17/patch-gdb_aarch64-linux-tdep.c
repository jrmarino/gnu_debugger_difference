--- gdb/aarch64-linux-tdep.c.orig	2026-01-30 14:07:34 UTC
+++ gdb/aarch64-linux-tdep.c
@@ -2625,14 +2625,14 @@ aarch64_linux_report_signal_info (struct
     return;
 
   CORE_ADDR fault_addr = 0;
-  long si_code = 0, si_errno = 0;
+  long db_si_code = 0, db_si_errno = 0;
 
   try
     {
       /* Sigcode tells us if the segfault is actually a memory tag
 	 violation.  */
-      si_code = parse_and_eval_long ("$_siginfo.si_code");
-      si_errno = parse_and_eval_long ("$_siginfo.si_errno");
+      db_si_code = parse_and_eval_long ("$_siginfo.si_code");
+      db_si_errno = parse_and_eval_long ("$_siginfo.si_errno");
 
       fault_addr
 	= parse_and_eval_long ("$_siginfo._sifields._sigfault.si_addr");
@@ -2645,9 +2645,9 @@ aarch64_linux_report_signal_info (struct
 
   const char *meaning;
 
-  if (si_code == SEGV_MTEAERR || si_code == SEGV_MTESERR)
+  if (db_si_code == SEGV_MTEAERR || db_si_code == SEGV_MTESERR)
     meaning = _("Memory tag violation");
-  else if (si_code == SEGV_CPERR && si_errno == 0)
+  else if (db_si_code == SEGV_CPERR && db_si_errno == 0)
     meaning = _("Guarded Control Stack error");
   else
     return;
@@ -2657,7 +2657,7 @@ aarch64_linux_report_signal_info (struct
   uiout->field_string ("sigcode-meaning", meaning);
 
   /* For synchronous faults, show additional information.  */
-  if (si_code == SEGV_MTESERR)
+  if (db_si_code == SEGV_MTESERR)
     {
       uiout->text (_(" while accessing address "));
       uiout->field_core_addr ("fault-addr", gdbarch, fault_addr);
@@ -2680,7 +2680,7 @@ aarch64_linux_report_signal_info (struct
 	  uiout->field_string ("logical-tag", hex_string (ltag));
 	}
     }
-  else if (si_code != SEGV_CPERR)
+  else if (db_si_code != SEGV_CPERR)
     {
       uiout->text ("\n");
       uiout->text (_("Fault address unavailable"));
