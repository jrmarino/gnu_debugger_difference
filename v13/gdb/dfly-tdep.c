/* Target-dependent code for FreeBSD, architecture-independent.

   Copyright (C) 2002-2023 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "defs.h"
#include "auxv.h"
#include "gdbcore.h"
#include "inferior.h"
#include "objfiles.h"
#include "regcache.h"
#include "regset.h"
#include "gdbthread.h"
#include "objfiles.h"
#include "xml-syscall.h"
#include <sys/socket.h>
#include <arpa/inet.h>

#include "elf-bfd.h"
#include "dfly-tdep.h"
#include "gcore-elf.h"

/* This enum is derived from DragonFly's <sys/signal.h>.  */

enum
  {
    DRAGONFLY_SIGHUP = 1,
    DRAGONFLY_SIGINT = 2,
    DRAGONFLY_SIGQUIT = 3,
    DRAGONFLY_SIGILL = 4,
    DRAGONFLY_SIGTRAP = 5,
    DRAGONFLY_SIGABRT = 6,
    DRAGONFLY_SIGEMT = 7,
    DRAGONFLY_SIGFPE = 8,
    DRAGONFLY_SIGKILL = 9,
    DRAGONFLY_SIGBUS = 10,
    DRAGONFLY_SIGSEGV = 11,
    DRAGONFLY_SIGSYS = 12,
    DRAGONFLY_SIGPIPE = 13,
    DRAGONFLY_SIGALRM = 14,
    DRAGONFLY_SIGTERM = 15,
    DRAGONFLY_SIGURG = 16,
    DRAGONFLY_SIGSTOP = 17,
    DRAGONFLY_SIGTSTP = 18,
    DRAGONFLY_SIGCONT = 19,
    DRAGONFLY_SIGCHLD = 20,
    DRAGONFLY_SIGTTIN = 21,
    DRAGONFLY_SIGTTOU = 22,
    DRAGONFLY_SIGIO = 23,
    DRAGONFLY_SIGXCPU = 24,
    DRAGONFLY_SIGXFSZ = 25,
    DRAGONFLY_SIGVTALRM = 26,
    DRAGONFLY_SIGPROF = 27,
    DRAGONFLY_SIGWINCH = 28,
    DRAGONFLY_SIGINFO = 29,
    DRAGONFLY_SIGUSR1 = 30,
    DRAGONFLY_SIGUSR2 = 31,
    DRAGONFLY_SIGTHR = 32,
  };

/* Constants for values of si_code as defined in DragonFly's
   <sys/_siginfo.h>.  */

#define	DFLY_SI_USER		0
#define	DFLY_SI_QUEUE		-1
#define	DFLY_SI_TIMER		-2
#define	DFLY_SI_ASYNCIO		-3
#define	DFLY_SI_MESGQ		-4

#define	DFLY_ILL_ILLOPC		1
#define	DFLY_ILL_ILLOPN		2
#define	DFLY_ILL_ILLADR		3
#define	DFLY_ILL_ILLTRP		4
#define	DFLY_ILL_PRVOPC		5
#define	DFLY_ILL_PRVREG		6
#define	DFLY_ILL_COPROC		7
#define	DFLY_ILL_BADSTK		8

#define	DFLY_BUS_ADRALN		1
#define	DFLY_BUS_ADRERR		2
#define	DFLY_BUS_OBJERR		3

#define	DFLY_SEGV_MAPERR	1
#define	DFLY_SEGV_ACCERR	2

#define	DFLY_FPE_INTOVF		1
#define	DFLY_FPE_INTDIV		2
#define	DFLY_FPE_FLTDIV		3
#define	DFLY_FPE_FLTOVF		4
#define	DFLY_FPE_FLTUND		5
#define	DFLY_FPE_FLTRES		6
#define	DFLY_FPE_FLTINV		7
#define	DFLY_FPE_FLTSUB		8

#define	DFLY_TRAP_BRKPT		1
#define	DFLY_TRAP_TRACE		2

#define	DFLY_CLD_EXITED		1
#define	DFLY_CLD_KILLED		2
#define	DFLY_CLD_DUMPED		3
#define	DFLY_CLD_TRAPPED	4
#define	DFLY_CLD_STOPPED	5
#define	DFLY_CLD_CONTINUED	6

#define	DFLY_POLL_IN		1
#define	DFLY_POLL_OUT		2
#define	DFLY_POLL_MSG		3
#define	DFLY_POLL_ERR		4
#define	DFLY_POLL_PRI		5
#define	DFLY_POLL_HUP		6

/* Constants for socket address families.  These match AF_* constants
   in <sys/socket.h>.  */

#define	DFLY_AF_UNIX		1
#define	DFLY_AF_INET		2
#define	DFLY_AF_INET6		28

/* Constants for socket types.  These match SOCK_* constants in
   <sys/socket.h>.  */

#define	DFLY_SOCK_STREAM	1
#define	DFLY_SOCK_DGRAM		2
#define	DFLY_SOCK_SEQPACKET	5

/* Constants for IP protocols.  These match IPPROTO_* constants in
   <netinet/in.h>.  */

#define DFLY_IPPROTO_IP		0
#define	DFLY_IPPROTO_ICMP	1
#define	DFLY_IPPROTO_TCP	6
#define	DFLY_IPPROTO_UDP	17

/* Socket address structures.  These have the same layout on all
   architectures.  In addition, multibyte fields such as IP
   addresses are always stored in network byte order.  */

struct dfly_sockaddr_in
{
  uint8_t sin_len;
  uint8_t sin_family;
  uint8_t sin_port[2];
  uint8_t sin_addr[4];
  char sin_zero[8];
};

struct dfly_sockaddr_in6
{
  uint8_t sin6_len;
  uint8_t sin6_family;
  uint8_t sin6_port[2];
  uint32_t sin6_flowinfo;
  uint8_t sin6_addr[16];
  uint32_t sin6_scope_id;
};

struct dfly_sockaddr_un
{
  uint8_t sun_len;
  uint8_t sun_family;
  char sun_path[104];
};

struct dfly_gdbarch_data
  {
    struct type *siginfo_type = nullptr;
  };

static const registry<gdbarch>::key<dfly_gdbarch_data>
     dfly_gdbarch_data_handle;

static struct dfly_gdbarch_data *
get_dfly_gdbarch_data (struct gdbarch *gdbarch)
{
  struct dfly_gdbarch_data *result = dfly_gdbarch_data_handle.get (gdbarch);
  if (result == nullptr)
    result = dfly_gdbarch_data_handle.emplace (gdbarch);
  return result;
}

struct dfly_pspace_data
{
  /* Offsets in the runtime linker's 'Obj_Entry' structure.  */
  LONGEST off_linkmap = 0;
  LONGEST off_tlsindex = 0;
  bool rtld_offsets_valid = false;

  /* vDSO mapping range.  */
  struct mem_range vdso_range {};

  /* Zero if the range hasn't been searched for, > 0 if a range was
     found, or < 0 if a range was not found.  */
  int vdso_range_p = 0;
};

/* Per-program-space data for FreeBSD architectures.  */
static const registry<program_space>::key<dfly_pspace_data>
  dfly_pspace_data_handle;

static struct dfly_pspace_data *
get_dfly_pspace_data (struct program_space *pspace)
{
  struct dfly_pspace_data *data;

  data = dfly_pspace_data_handle.get (pspace);
  if (data == NULL)
    data = dfly_pspace_data_handle.emplace (pspace);

  return data;
}

/* Implement the "gdb_signal_from_target" gdbarch method.  */

static enum gdb_signal
dfly_gdb_signal_from_target (struct gdbarch *gdbarch, int signal)
{
  switch (signal)
    {
    case 0:
      return GDB_SIGNAL_0;

    case DRAGONFLY_SIGHUP:
      return GDB_SIGNAL_HUP;

    case DRAGONFLY_SIGINT:
      return GDB_SIGNAL_INT;

    case DRAGONFLY_SIGQUIT:
      return GDB_SIGNAL_QUIT;

    case DRAGONFLY_SIGILL:
      return GDB_SIGNAL_ILL;

    case DRAGONFLY_SIGTRAP:
      return GDB_SIGNAL_TRAP;

    case DRAGONFLY_SIGABRT:
      return GDB_SIGNAL_ABRT;

    case DRAGONFLY_SIGEMT:
      return GDB_SIGNAL_EMT;

    case DRAGONFLY_SIGFPE:
      return GDB_SIGNAL_FPE;

    case DRAGONFLY_SIGKILL:
      return GDB_SIGNAL_KILL;

    case DRAGONFLY_SIGBUS:
      return GDB_SIGNAL_BUS;

    case DRAGONFLY_SIGSEGV:
      return GDB_SIGNAL_SEGV;

    case DRAGONFLY_SIGSYS:
      return GDB_SIGNAL_SYS;

    case DRAGONFLY_SIGPIPE:
      return GDB_SIGNAL_PIPE;

    case DRAGONFLY_SIGALRM:
      return GDB_SIGNAL_ALRM;

    case DRAGONFLY_SIGTERM:
      return GDB_SIGNAL_TERM;

    case DRAGONFLY_SIGURG:
      return GDB_SIGNAL_URG;

    case DRAGONFLY_SIGSTOP:
      return GDB_SIGNAL_STOP;

    case DRAGONFLY_SIGTSTP:
      return GDB_SIGNAL_TSTP;

    case DRAGONFLY_SIGCONT:
      return GDB_SIGNAL_CONT;

    case DRAGONFLY_SIGCHLD:
      return GDB_SIGNAL_CHLD;

    case DRAGONFLY_SIGTTIN:
      return GDB_SIGNAL_TTIN;

    case DRAGONFLY_SIGTTOU:
      return GDB_SIGNAL_TTOU;

    case DRAGONFLY_SIGIO:
      return GDB_SIGNAL_IO;

    case DRAGONFLY_SIGXCPU:
      return GDB_SIGNAL_XCPU;

    case DRAGONFLY_SIGXFSZ:
      return GDB_SIGNAL_XFSZ;

    case DRAGONFLY_SIGVTALRM:
      return GDB_SIGNAL_VTALRM;

    case DRAGONFLY_SIGPROF:
      return GDB_SIGNAL_PROF;

    case DRAGONFLY_SIGWINCH:
      return GDB_SIGNAL_WINCH;

    case DRAGONFLY_SIGINFO:
      return GDB_SIGNAL_INFO;

    case DRAGONFLY_SIGUSR1:
      return GDB_SIGNAL_USR1;

    case DRAGONFLY_SIGUSR2:
      return GDB_SIGNAL_USR2;

    /* SIGTHR is the same as SIGLWP on FreeBSD. */
    case DRAGONFLY_SIGTHR:
      return GDB_SIGNAL_LWP;

    case DRAGONFLY_SIGLIBRT:
      return GDB_SIGNAL_LIBRT;
    }

  return GDB_SIGNAL_UNKNOWN;
}

/* Implement the "gdb_signal_to_target" gdbarch method.  */

static int
dfly_gdb_signal_to_target (struct gdbarch *gdbarch,
		enum gdb_signal signal)
{
  switch (signal)
    {
    case GDB_SIGNAL_0:
      return 0;

    case GDB_SIGNAL_HUP:
      return DRAGONFLY_SIGHUP;

    case GDB_SIGNAL_INT:
      return DRAGONFLY_SIGINT;

    case GDB_SIGNAL_QUIT:
      return DRAGONFLY_SIGQUIT;

    case GDB_SIGNAL_ILL:
      return DRAGONFLY_SIGILL;

    case GDB_SIGNAL_TRAP:
      return DRAGONFLY_SIGTRAP;

    case GDB_SIGNAL_ABRT:
      return DRAGONFLY_SIGABRT;

    case GDB_SIGNAL_EMT:
      return DRAGONFLY_SIGEMT;

    case GDB_SIGNAL_FPE:
      return DRAGONFLY_SIGFPE;

    case GDB_SIGNAL_KILL:
      return DRAGONFLY_SIGKILL;

    case GDB_SIGNAL_BUS:
      return DRAGONFLY_SIGBUS;

    case GDB_SIGNAL_SEGV:
      return DRAGONFLY_SIGSEGV;

    case GDB_SIGNAL_SYS:
      return DRAGONFLY_SIGSYS;

    case GDB_SIGNAL_PIPE:
      return DRAGONFLY_SIGPIPE;

    case GDB_SIGNAL_ALRM:
      return DRAGONFLY_SIGALRM;

    case GDB_SIGNAL_TERM:
      return DRAGONFLY_SIGTERM;

    case GDB_SIGNAL_URG:
      return DRAGONFLY_SIGURG;

    case GDB_SIGNAL_STOP:
      return DRAGONFLY_SIGSTOP;

    case GDB_SIGNAL_TSTP:
      return DRAGONFLY_SIGTSTP;

    case GDB_SIGNAL_CONT:
      return DRAGONFLY_SIGCONT;

    case GDB_SIGNAL_CHLD:
      return DRAGONFLY_SIGCHLD;

    case GDB_SIGNAL_TTIN:
      return DRAGONFLY_SIGTTIN;

    case GDB_SIGNAL_TTOU:
      return DRAGONFLY_SIGTTOU;

    case GDB_SIGNAL_IO:
      return DRAGONFLY_SIGIO;

    case GDB_SIGNAL_XCPU:
      return DRAGONFLY_SIGXCPU;

    case GDB_SIGNAL_XFSZ:
      return DRAGONFLY_SIGXFSZ;

    case GDB_SIGNAL_VTALRM:
      return DRAGONFLY_SIGVTALRM;

    case GDB_SIGNAL_PROF:
      return DRAGONFLY_SIGPROF;

    case GDB_SIGNAL_WINCH:
      return DRAGONFLY_SIGWINCH;

    case GDB_SIGNAL_INFO:
      return DRAGONFLY_SIGINFO;

    case GDB_SIGNAL_USR1:
      return DRAGONFLY_SIGUSR1;

    case GDB_SIGNAL_USR2:
      return DRAGONFLY_SIGUSR2;

    case GDB_SIGNAL_LWP:
      return DRAGONFLY_SIGTHR;

    case GDB_SIGNAL_LIBRT:
      return DRAGONFLY_SIGLIBRT;
    }

  if (signal >= GDB_SIGNAL_REALTIME_65
      && signal <= GDB_SIGNAL_REALTIME_126)
    {
      int offset = signal - GDB_SIGNAL_REALTIME_65;

      return DRAGONFLY_SIGRTMIN + offset;
    }

  return -1;
}


/* Return description of signal code or nullptr.  */

#if 0
static const char *
dfly_signal_cause (enum gdb_signal siggnal, int code)
{
  /* Signal-independent causes.  */
  switch (code)
    {
    case DFLY_SI_USER:
      return _("Sent by kill()");
    case DFLY_SI_QUEUE:
      return _("Sent by sigqueue()");
    case DFLY_SI_TIMER:
      return _("Timer expired");
    case DFLY_SI_ASYNCIO:
      return _("Asynchronous I/O request completed");
    case DFLY_SI_MESGQ:
      return _("Message arrived on empty message queue");
    }

  switch (siggnal)
    {
    case GDB_SIGNAL_ILL:
      switch (code)
	{
	case DFLY_ILL_ILLOPC:
	  return _("Illegal opcode");
	case DFLY_ILL_ILLOPN:
	  return _("Illegal operand");
	case DFLY_ILL_ILLADR:
	  return _("Illegal addressing mode");
	case DFLY_ILL_ILLTRP:
	  return _("Illegal trap");
	case DFLY_ILL_PRVOPC:
	  return _("Privileged opcode");
	case DFLY_ILL_PRVREG:
	  return _("Privileged register");
	case DFLY_ILL_COPROC:
	  return _("Coprocessor error");
	case DFLY_ILL_BADSTK:
	  return _("Internal stack error");
	}
      break;
    case GDB_SIGNAL_BUS:
      switch (code)
	{
	case DFLY_BUS_ADRALN:
	  return _("Invalid address alignment");
	case DFLY_BUS_ADRERR:
	  return _("Address not present");
	case DFLY_BUS_OBJERR:
	  return _("Object-specific hardware error");
	}
      break;
    case GDB_SIGNAL_SEGV:
      switch (code)
	{
	case DFLY_SEGV_MAPERR:
	  return _("Address not mapped to object");
	case DFLY_SEGV_ACCERR:
	  return _("Invalid permissions for mapped object");
	}
      break;
    case GDB_SIGNAL_FPE:
      switch (code)
	{
	case DFLY_FPE_INTOVF:
	  return _("Integer overflow");
	case DFLY_FPE_INTDIV:
	  return _("Integer divide by zero");
	case DFLY_FPE_FLTDIV:
	  return _("Floating point divide by zero");
	case DFLY_FPE_FLTOVF:
	  return _("Floating point overflow");
	case DFLY_FPE_FLTUND:
	  return _("Floating point underflow");
	case DFLY_FPE_FLTRES:
	  return _("Floating point inexact result");
	case DFLY_FPE_FLTINV:
	  return _("Invalid floating point operation");
	case DFLY_FPE_FLTSUB:
	  return _("Subscript out of range");
	}
      break;
    case GDB_SIGNAL_TRAP:
      switch (code)
	{
	case DFLY_TRAP_BRKPT:
	  return _("Breakpoint");
	case DFLY_TRAP_TRACE:
	  return _("Trace trap");
	}
      break;
    case GDB_SIGNAL_CHLD:
      switch (code)
	{
	case DFLY_CLD_EXITED:
	  return _("Child has exited");
	case DFLY_CLD_KILLED:
	  return _("Child has terminated abnormally");
	case DFLY_CLD_DUMPED:
	  return _("Child has dumped core");
	case DFLY_CLD_TRAPPED:
	  return _("Traced child has trapped");
	case DFLY_CLD_STOPPED:
	  return _("Child has stopped");
	case DFLY_CLD_CONTINUED:
	  return _("Stopped child has continued");
	}
      break;
    case GDB_SIGNAL_POLL:
      switch (code)
	{
	case DFLY_POLL_IN:
	  return _("Data input available");
	case DFLY_POLL_OUT:
	  return _("Output buffers available");
	case DFLY_POLL_MSG:
	  return _("Input message available");
	case DFLY_POLL_ERR:
	  return _("I/O error");
	case DFLY_POLL_PRI:
	  return _("High priority input available");
	case DFLY_POLL_HUP:
	  return _("Device disconnected");
	}
      break;
    }

  return nullptr;
}
#endif

/* Print descriptions of DragonFly-specific AUXV entries to FILE.  */

static void
dfly_print_auxv_entry (struct gdbarch *gdbarch, struct ui_file *file,
		       CORE_ADDR type, CORE_ADDR val)
{
  const char *name = "???";
  const char *description = "";
  enum auxv_format format = AUXV_FORMAT_HEX;

  switch (type)
    {
    case AT_NULL:
    case AT_IGNORE:
    case AT_EXECFD:
    case AT_PHDR:
    case AT_PHENT:
    case AT_PHNUM:
    case AT_PAGESZ:
    case AT_BASE:
    case AT_FLAGS:
    case AT_ENTRY:
    case AT_NOTELF:
    case AT_UID:
    case AT_EUID:
    case AT_GID:
    case AT_EGID:
      default_print_auxv_entry (gdbarch, file, type, val);
      return;
#define _TAGNAME(tag) #tag
#define TAGNAME(tag) _TAGNAME(AT_##tag)
#define TAG(tag, text, kind) \
      case AT_FREEBSD_##tag: name = TAGNAME(tag); description = text; format = kind; break
      TAG (EXECPATH, _("Executable path"), AUXV_FORMAT_STR);
      TAG (CANARY, _("Canary for SSP"), AUXV_FORMAT_HEX);
      TAG (CANARYLEN, ("Length of the SSP canary"), AUXV_FORMAT_DEC);
      TAG (OSRELDATE, _("OSRELDATE"), AUXV_FORMAT_DEC);
      TAG (NCPUS, _("Number of CPUs"), AUXV_FORMAT_DEC);
      TAG (PAGESIZES, _("Pagesizes"), AUXV_FORMAT_HEX);
      TAG (PAGESIZESLEN, _("Number of pagesizes"), AUXV_FORMAT_DEC);
      TAG (STACKPROT, _("Initial stack protection"), AUXV_FORMAT_HEX);
      TAG (EHDRFLAGS, _("ELF header e_flags"), AUXV_FORMAT_HEX);
      TAG (HWCAP, _("Machine-dependent CPU capability hints"), AUXV_FORMAT_HEX);
      TAG (HWCAP2, _("Extension of AT_HWCAP"), AUXV_FORMAT_HEX);
      TAG (BSDFLAGS, _("ELF BSD flags"), AUXV_FORMAT_HEX);
      TAG (ARGC, _("Argument count"), AUXV_FORMAT_DEC);
      TAG (ARGV, _("Argument vector"), AUXV_FORMAT_HEX);
      TAG (ENVC, _("Environment count"), AUXV_FORMAT_DEC);
      TAG (ENVV, _("Environment vector"), AUXV_FORMAT_HEX);
      TAG (PS_STRINGS, _("Pointer to ps_strings"), AUXV_FORMAT_HEX);
      TAG (FXRNG, _("Pointer to root RNG seed version"), AUXV_FORMAT_HEX);
      TAG (KPRELOAD, _("Base address of vDSO"), AUXV_FORMAT_HEX);
      TAG (USRSTACKBASE, _("Top of user stack"), AUXV_FORMAT_HEX);
      TAG (USRSTACKLIM, _("Grow limit of user stack"), AUXV_FORMAT_HEX);
    }

  fprint_auxv_entry (file, name, description, format, type, val);
}


/* See fbsd-tdep.h.  */

CORE_ADDR
dfly_skip_solib_resolver (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  struct bound_minimal_symbol msym = lookup_bound_minimal_symbol ("_rtld_bind");
  if (msym.minsym != nullptr && msym.value_address () == pc)
    return frame_unwind_caller_pc (get_current_frame ());

  return 0;
}



/* To be called from GDB_OSABI_DRAGONFLY handlers. */

void
dfly_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
#if 0
  set_gdbarch_core_pid_to_str (gdbarch, dfly_core_pid_to_str);
  set_gdbarch_core_thread_name (gdbarch, dfly_core_thread_name);
  set_gdbarch_core_xfer_siginfo (gdbarch, dfly_core_xfer_siginfo);
  set_gdbarch_make_corefile_notes (gdbarch, dfly_make_corefile_notes);
  set_gdbarch_core_info_proc (gdbarch, dfly_core_info_proc);
  set_gdbarch_get_siginfo_type (gdbarch, dfly_get_siginfo_type);
#endif
  set_gdbarch_print_auxv_entry (gdbarch, dfly_print_auxv_entry);
  set_gdbarch_gdb_signal_from_target (gdbarch, dfly_gdb_signal_from_target);
  set_gdbarch_gdb_signal_to_target (gdbarch, dfly_gdb_signal_to_target);
  set_gdbarch_skip_solib_resolver (gdbarch, dfly_skip_solib_resolver);

#if 0
  set_gdbarch_report_signal_info (gdbarch, dfly_report_signal_info);
  set_gdbarch_vsyscall_range (gdbarch, dfly_vsyscall_range);

  /* `catch syscall' */
  set_xml_syscall_file_name (gdbarch, "syscalls/dragonfly.xml");
  set_gdbarch_get_syscall_number (gdbarch, dfly_get_syscall_number);
#endif
}
