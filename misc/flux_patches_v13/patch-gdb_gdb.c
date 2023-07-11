--- gdb/gdb.c.orig	2023-07-09 21:51:59 UTC
+++ gdb/gdb.c
@@ -28,6 +28,12 @@ main (int argc, char **argv)
   memset (&args, 0, sizeof args);
   args.argc = argc;
   args.argv = argv;
-  args.interpreter_p = INTERP_CONSOLE;
+  if (strncmp(basename(argv[0]), "insight", 7) == 0) {
+    args.interpreter_p = "insight";
+  } else if (strncmp(basename(argv[0]), "gdbtui", 6) == 0) {
+    args.interpreter_p = INTERP_TUI;
+  } else {
+    args.interpreter_p = INTERP_CONSOLE;
+  }
   return gdb_main (&args);
 }
