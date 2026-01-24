--- include/elf/common.h.orig
+++ include/elf/common.h
@@ -1080,6 +1080,7 @@
 /* Values for FreeBSD .note.ABI-tag notes.  Note name is "FreeBSD".  */
 
 #define NT_FREEBSD_ABI_TAG	1
+#define NT_DRAGONFLY_ABI_TAG	1
 
 /* Values for FDO .note.package notes as defined on https://systemd.io/COREDUMP_PACKAGE_METADATA/  */
 #define FDO_PACKAGING_METADATA	0xcafe1a7e
