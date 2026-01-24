/* Target-dependent code for DragonFly, architecture-independent.

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
#include "regcache.h"
#include "regset.h"
#include "gdbthread.h"
#include "objfiles.h"
#include "xml-syscall.h"
#include <sys/socket.h>
#include <arpa/inet.h>

#include "elf-bfd.h"
#include "dfly-tdep.h"

/* This enum is derived from FreeBSD's <sys/signal.h>.  */

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
    DRAGONFLY_SIGCKPT = 33,
    DRAGONFLY_SIGCKPTEXIT = 34,
  };


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

#define	DFLY_IPPROTO_ICMP	1
#define	DFLY_IPPROTO_TCP	6
#define	DFLY_IPPROTO_UDP	17

/* Socket address structures.  These have the same layout on all
   DragonFly architectures.  In addition, multibyte fields such as IP
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

struct dfly_pspace_data
{
  /* Offsets in the runtime linker's 'Obj_Entry' structure.  */
  LONGEST off_linkmap = 0;
  LONGEST off_tlsindex = 0;
  bool rtld_offsets_valid = false;
};

/* Per-program-space data for FreeBSD architectures.  */
static const registry<program_space>::key<dfly_pspace_data>
  dfly_pspace_data_handle;

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

    }

  return -1;
}

/* To be called from GDB_OSABI_DRAGONFLY handlers. */

void
dfly_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  set_gdbarch_gdb_signal_from_target (gdbarch, dfly_gdb_signal_from_target);
  set_gdbarch_gdb_signal_to_target (gdbarch, dfly_gdb_signal_to_target);

}
