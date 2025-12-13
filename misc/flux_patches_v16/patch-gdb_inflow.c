--- gdb/inflow.c.orig	2023-07-09 21:46:24 UTC
+++ gdb/inflow.c
@@ -854,7 +854,10 @@ static void
 pass_signal (int signo)
 {
 #ifndef _WIN32
-  kill (inferior_ptid.pid (), SIGINT);
+  if (inferior_ptid.pid () )
+    kill (inferior_ptid.pid (), SIGINT);
+  else
+    kill (current_inferior ()->pid, SIGINT);
 #endif
 }
 
