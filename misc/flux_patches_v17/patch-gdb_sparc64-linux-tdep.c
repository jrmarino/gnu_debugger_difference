--- gdb/sparc64-linux-tdep.c.orig
+++ gdb/sparc64-linux-tdep.c
@@ -130,14 +130,14 @@ sparc64_linux_report_signal_info (struct
     return;
 
   CORE_ADDR addr = 0;
-  long si_code = 0;
+  long db_si_code = 0;
 
   try
     {
-      /* Evaluate si_code to see if the segfault is ADI related.  */
-      si_code = parse_and_eval_long ("$_siginfo.si_code\n");
+      /* Evaluate db_si_code to see if the segfault is ADI related.  */
+      db_si_code = parse_and_eval_long ("$_siginfo.si_code\n");
 
-      if (si_code >= SEGV_ACCADI && si_code <= SEGV_ADIPERR)
+      if (db_si_code >= SEGV_ACCADI && db_si_code <= SEGV_ADIPERR)
 	addr = parse_and_eval_long ("$_siginfo._sifields._sigfault.si_addr");
     }
   catch (const gdb_exception_error &exception)
@@ -146,7 +146,7 @@ sparc64_linux_report_signal_info (struct
     }
 
   /* Print out ADI event based on sig_code value */
-  switch (si_code)
+  switch (db_si_code)
     {
     case SEGV_ACCADI:	/* adi not enabled */
       uiout->text ("\n");
