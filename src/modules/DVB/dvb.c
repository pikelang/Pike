/*
 * DVB module
 *
 * Creator: Honza Petrous <hop@unibase.cz>
 *
 * $Id: dvb.c,v 1.7 2002/10/01 21:25:42 nilsson Exp $
 *
 * Credits:
 *  + Tuner zapping code based on 'szap' app from linux DVB driver
 *    package [ szap.c: (c) 2001 Johannes Stezenbach js@convergence.de ]
 *  + PMT,PAT,ECM parsing code based on 'mgcam' app [ by badfish :]
 *  + PES parsing code based on 'mpegtools' app from linux DVB driver
 *    package [ (c) 2000, 2001 Marcus Metzler marcus@convergence.de ]
 *
 * Distro: aconfig.h (automake-> config.h.in), configure.in, Makefile.in
 *
 * Todo:
 * 	- remove crc32 table and use Gz.crc32() call
 * 	- "Frontend", "Mux" ...
 */

#include "config.h"


#ifdef HAVE_DVB

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

#include <stdint.h>
#include <sys/time.h>

#if HAVE_DVB > 19
#  include <linux/dvb/version.h>
#  include <linux/dvb/sec.h>
#  include <linux/dvb/frontend.h>
#  include <linux/dvb/dmx.h>
#  include <linux/dvb/audio.h>
#  define SECDEVICE "/dev/dvb/sec"
#  define FRONTENDDEVICE "/dev/dvb/frontend"
#  define DEMUXDEVICE "/dev/dvb/demux"
#  define AUDIODEVICE "/dev/dvb/audio"
#else
#  include <ost/sec.h>
#  include <ost/frontend.h>
#  include <ost/dmx.h>
#  include <ost/audio.h>
#  define SECDEVICE "/dev/ost/sec"
#  define FRONTENDDEVICE "/dev/ost/frontend"
#  define DEMUXDEVICE "/dev/ost/demux"
#  define AUDIODEVICE "/dev/ost/audio"
#endif

#include "interpret.h"
#include "svalue.h"
#include "stralloc.h"
#include "array.h"
#include "object.h"
#include "mapping.h"
#include "pike_macros.h"
#include "threads.h"
#include "fd_control.h"
#include "module_support.h"
#include "builtin_functions.h"
#include "operators.h"

#include "dvb.h"
#endif /* HAVE_DVB */

/* MUST BE LAST */
#include "module_magic.h"

#ifdef HAVE_DVB

/* WARNING: It is a design limit of DVB-S full cards! */
#define MAX_PES_FD	8

#define MAX_DVB_READ_SIZE 4096
#define MAX_ERR_LEN	160

#undef THREAD_ALLOW
#define THREAD_ALLOW
#undef THREAD_DISALLOW
#define THREAD_DISALLOW

static struct program *dvb_program;

struct ECMINFO {
  struct ECMINFO *next;
  char *name;
  int system;
  int ecm_pid;
  int id;
};

typedef struct {
  int			 cardn;
  int			 fefd;
  unsigned int		 pescnt;
  int			 pesfd[MAX_PES_FD];
  unsigned int		 pesid[MAX_PES_FD];
  unsigned int		 pestype[MAX_PES_FD];
  dvb_avpacket		 pespkt[MAX_PES_FD];
  unsigned int		 pesbuflen[MAX_PES_FD];
  struct svalue		 pesfcb[MAX_PES_FD];
  dvb_p2p		 p;
  struct ECMINFO	*ecminfo;
  char			 low_errmsg[MAX_ERR_LEN+1];
} dvb_data;

#define DVB	((dvb_data *)Pike_fp->current_storage)
#define THISOBJ (Pike_fp->current_object)

#define IOCTL(X,Y,Z) 							\
  if( ioctl( X, #Y, Z ) ) {						\
    fprintf(stderr, "ioctl " #Y " failed: %s\n", strerror(errno) );	\
    push_int(0);							\
  } else 								\
    push_int(1);

/*! @module DVB
 */

/*! @class dvb
 *!
 *! Implements Digital Video Broadcasting interface
 *!
 *! @note
 *!  Only Linux version is supported.
 */

/*! @decl void create(int card_number)
 *!
 *! Create a DVB object.
 *!
 *! @param card_number
 *!   The number of card equipment.
 *!
 *! @note
 *!   The number spcecifies which device will be opened.
 *!   Ie. /dev/ost/demux0, /dev/ost/demux1 ... for
 *!   DVB v0.9.4  or /dev/dvb/demux0, /dev/dvb/demux1 ...
 *!   for versions 2.0+
 */
static void f_create(INT32 args) {

  int fefd;
  char *devname;

  if(DVB->cardn != -1)
    Pike_error("Create already called!\n");

  DVB->cardn = 0;
  if(args)
    if(Pike_sp[-1].type != T_INT)
      Pike_error("Invalid argument 1, expected int.\n");
    else
      DVB->cardn = (u_short)Pike_sp[-1].u.integer;

  if((devname = malloc(sizeof(FRONTENDDEVICE)+1)) == NULL)
      Pike_error("Internal error: can't malloc buffer.\n");
  strcpy(devname, FRONTENDDEVICE);
  if(DVB->cardn) {
    devname[strlen(FRONTENDDEVICE)] = DVB->cardn + '0';
    devname[strlen(FRONTENDDEVICE)+1] = '\0';
  }

  fefd = open (devname, O_RDWR | O_NONBLOCK);
  if (fefd < 0) {
      DVB->cardn = -1;
      Pike_error("Opening frontend '%s' failed.\n", devname);
  }
  DVB->fefd = fefd;

  /* Make sure this fd gets closed on exec. */
  set_close_on_exec(fefd, 1);

} /* create */

/* LNB hi/lo band switch frequency in kHz */
#define SWITCHFREQ 11700000
/* LNB hi/lo band local oscillator frequencies in kHz */
#define LOF_HI 10600000
#define LOF_LO 9750000


/*! @decl mapping|int fe_status()
 *!
 *! Return status of a DVB object's frondend device.
 *!
 *!  @returns
 *!   The resulting mapping contains the following fields:
 *!   @mapping
 *!     @member string "power"
 *!       If 1 the frontend is powered up and is ready to be used.
 *!     @member string "signal"
 *!       If 1 the frontend detects a signal above a normal noise level
 *!     @member string "lock"
 *!       If 1 the frontend successfully locked to a DVB signal
 *!     @member string "carrier"
 *!       If 1 carrier dectected in signal
 *!     @member string "biterbi"
 *!       If 1 then lock at viterbi state
 *!     @member string "sync"
 *!       If 1 then TS sync byte detected
 *!     @member string "tuner_lock"
 *!       If 1 then tuner has a frequency lock
 *!   @endmapping
 */
static void f_fe_status(INT32 args) {

  uint32_t status;
  int cnt = 0, ret;

  pop_n_elems(args);
  THREADS_ALLOW();
  ret = ioctl(DVB->fefd, FE_READ_STATUS, &status);
  THREADS_DISALLOW();
  if(ret < 0)
    push_int(0);
  else {
    push_text("power"); push_int(!!(status & ~FE_HAS_POWER));
    push_text("signal"); push_int(!!(status & ~FE_HAS_SIGNAL));
    /*push_text("spectrum_inverse"); push_int(status & ~FE_HAS_SPECTRUM_INV);*/
    push_text("lock"); push_int(!!(status & ~FE_HAS_LOCK));
    push_text("carrier"); push_int(!!(status & ~FE_HAS_CARRIER));
    push_text("viterbi"); push_int(!!(status & ~FE_HAS_VITERBI));
    push_text("sync"); push_int(!!(status & ~FE_HAS_SYNC));
    push_text("tuner_lock"); push_int(!!(status & ~FE_TUNER_HAS_LOCK));
    cnt = 7;
    THREADS_ALLOW();
    ret = ioctl(DVB->fefd, FE_READ_BER, &status);
    THREADS_DISALLOW();
    if(ret > -1) {
      push_text("ber"); push_int(status);
      cnt++;
    }
    THREADS_ALLOW();
    ret = ioctl(DVB->fefd, FE_READ_SNR, &status);
    THREADS_DISALLOW();
    if(ret > -1) {
      push_text("snr"); push_int(status);
      cnt++;
    }
    THREADS_ALLOW();
    ret = ioctl(DVB->fefd, FE_READ_SIGNAL_STRENGTH, &status);
    THREADS_DISALLOW();
    if(ret > -1) {
      push_text("signal_strength"); push_int(status);
      cnt++;
    }

    f_aggregate_mapping(2 * cnt);
  }
}

/*! @decl mapping fe_info()
 *!
 *! Return info of a DVB object's frondend device.
 *!
 *! @note
 *!   The information heavily depends on driver. Many fields
 *!   contain dumb values.
 */
static void f_fe_info(INT32 args) {

  FrontendInfo info;
  int ret;

  pop_n_elems(args);
  THREADS_ALLOW();
  ret = ioctl(DVB->fefd, FE_GET_INFO, &info);
  THREADS_DISALLOW();
  if(ret < 0)
    push_int(0);
  else {
    push_text("frequency");
      push_text("min"); push_int(info.minFrequency);
      push_text("max"); push_int(info.maxFrequency);
      f_aggregate_mapping(2 * 2);
    push_text("sr");
      push_text("min"); push_int(info.minSymbolRate);
      push_text("max"); push_int(info.maxSymbolRate);
      f_aggregate_mapping(2 * 2);
    push_text("hardware");
      push_text("type"); push_int(info.hwType);
      push_text("version"); push_int(info.hwVersion);
      f_aggregate_mapping(2 * 2);
    f_aggregate_mapping(2 * 3);
  }
}

/* digital satellite equipment control,
 * specification is available from http://www.eutelsat.com/ 
 */
static int diseqc (int secfd, int sat_no, int pol, int hi_lo) {
  struct secCmdSequence secseq;
  struct secCommand scmd;

  scmd.type = SEC_CMDTYPE_DISEQC;
  scmd.u.diseqc.addr = 0x10;		/* any switch */
  scmd.u.diseqc.cmd = 0x38;		/* set port group 0 */
  scmd.u.diseqc.numParams = 1;
  /* param: high nibble: reset bits, low nibble set bits,
   * bits are: option, position, polarizaion, band
   */
  scmd.u.diseqc.params[0] = 0xF0 | (((sat_no * 4) & 0x0F)
                                    | (hi_lo?1:0) | (pol?0:2));

  /* tone/volt for backwards compatiblity with non-diseqc LNBs */
  secseq.voltage = pol ? SEC_VOLTAGE_13 : SEC_VOLTAGE_18;
  secseq.continuousTone = hi_lo ? SEC_TONE_ON : SEC_TONE_OFF;
  secseq.miniCommand = SEC_MINI_NONE;
  secseq.numCommands = 1;
  secseq.commands = &scmd;

  if (ioctl (secfd, SEC_SEND_SEQUENCE, &secseq) == -1)
    return 0; 
  return 1;
}

static int do_tune(int fefd, uint ifreq, uint sr)
{
  FrontendParameters tuneto;
  FrontendEvent ev;
  struct pollfd pfd;
  int prc, ret;

  /* discard stale QPSK events */
  while (1) {
      THREADS_ALLOW();
      ret = ioctl (fefd, FE_GET_EVENT, &ev);
      THREADS_DISALLOW();
      if (ret == -1)
	break;
  }

  tuneto.Frequency = ifreq;
  tuneto.u.qpsk.SymbolRate = sr;
  tuneto.u.qpsk.FEC_inner = FEC_AUTO;
  THREADS_ALLOW();
  ret = ioctl (fefd, FE_SET_FRONTEND, &tuneto);
  THREADS_DISALLOW();
  if (ret == -1) {
      snprintf (DVB->low_errmsg, MAX_ERR_LEN, "FE_SET_FRONTEND failed.\n");
      return 0;
  }

  /* wait for tunig to complete, with timout */
  pfd.fd = fefd;
  pfd.events = POLLIN | POLLPRI;
  prc = poll (&pfd, 1, 5000);
  if (prc == -1) {
      snprintf (DVB->low_errmsg, MAX_ERR_LEN, "FE_GET_EVENT failed.\n");
      return 0;
  } else if (prc == 0) {
      snprintf (DVB->low_errmsg, MAX_ERR_LEN, "FE_GET_EVENT timed out.\n");
      return 0;
  }

  if (ioctl (fefd, FE_GET_EVENT, &ev) == -1) {
      snprintf (DVB->low_errmsg, MAX_ERR_LEN, "FE_GET_EVENT failed.\n");
      return 0;
  }

  if (ev.type != FE_COMPLETION_EV) {
      snprintf (DVB->low_errmsg, MAX_ERR_LEN, "tuning failed\n");
      return 0;
  }

  return 1;
}


/*! @decl int tune(int(0..3) lnb, int freq, int(0..1)|string pol, int sr)
 *!
 *!   Tunes to apropriate transponder's parameters and optionally
 *!   sets PID(s).
 *!
 *! @param lnb
 *!  DiSeQc number of LNB.
 *!
 *! @param freq
 *!  Frequency divided by 1000.
 *!
 *! @param pol
 *!  Polarization. @tt{0@} or @tt{"v"@} for vertical type,
 *!  @tt{1@} or @tt{"h"@} for horizontal one.
 *!
 *! @param sr
 *!  The service rate parameter.
 *!
 */
static void f_zap(INT32 args) {
  int secfd;
  uint ifreq;
  int hiband, result;
  FrontendInfo fe_info;

  int satno;
  uint freq;
  int pol;
  uint sr;

  check_all_args("DVB.dvb->tune", args, BIT_INT, BIT_INT, BIT_INT | BIT_STRING,
		  			BIT_INT, 0);

  sr = (u_short)Pike_sp[-1].u.integer * 1000;
  Pike_sp--;

  if(Pike_sp[-1].type == T_INT)
      pol = (u_short)Pike_sp[-1].u.integer;
  else
      pol = Pike_sp[-1].u.string->str[0] == 'V' ||
	    Pike_sp[-1].u.string->str[0] == 'v';
  Pike_sp--;

  freq = (u_short)Pike_sp[-1].u.integer * 1000;
  Pike_sp--;

  satno = (u_short)Pike_sp[-1].u.integer;

  secfd = open (SECDEVICE, O_RDWR);
  if (secfd == -1) {
      Pike_error ("opening SEC failed\n");
  }
  THREADS_ALLOW();
  result = ioctl (DVB->fefd, FE_GET_INFO, &fe_info);
  THREADS_DISALLOW();
  if (result == -1 || fe_info.type != FE_QPSK) {
      close (secfd);
      Pike_error("ioctl on fefd failed\n");
  }

  hiband = (freq >= SWITCHFREQ);
  if (hiband)
    ifreq = freq - LOF_HI;
  else
    ifreq = freq - LOF_LO;

  result = 0;
  if (diseqc (secfd, satno, pol, hiband))
    if (do_tune (DVB->fefd, ifreq, sr))
	  result = 1;

  close (secfd);

  if(!result)
    Pike_error(DVB->low_errmsg);

  push_int(result);
}


/*! @decl mapping|int get_pids()
 *!
 *! Returns mapping with info of currently tuned program's pids.
 *!
 *! @seealso
 *!   @[tune()]
 */
static void f_get_pids(INT32 args) {

  dvb_pid_t pids[7];
  int cnt = 0, dmx, ret;

  pop_n_elems(args);
  if(!DVB->pescnt) {
    dmx = open (DEMUXDEVICE, O_RDWR | O_NONBLOCK);
    if (dmx < 0)
      Pike_error("Opening demux failed.\n");
  } else
    // FIXME: for which PMT ?
    dmx = DVB->pesfd[0];
  /* get the current PID settings */
  THREADS_ALLOW();
  ret = ioctl(dmx, DMX_GET_PES_PIDS, &pids);
  THREADS_DISALLOW();
  if (ret) {
    Pike_error("GET PIDS failed.\n");
  }

  if(DVB->cardn != -1) {
    push_text("audio");		push_int( pids[DMX_PES_AUDIO] & 0x1fff );
    cnt++;
    push_text("video");		push_int( pids[DMX_PES_VIDEO] & 0x1fff );
    cnt++;
    push_text("teletext");	push_int( pids[DMX_PES_TELETEXT] & 0x1fff );
    cnt++;
    push_text("subtitle");	push_int( pids[DMX_PES_SUBTITLE] & 0x1fff );
    cnt++;
    push_text("pcr");		push_int( pids[DMX_PES_PCR] & 0x1fff );
    cnt++;
    push_text("other");		push_int( pids[DMX_PES_OTHER] & 0x1fff );
    cnt++;
    if(cnt)
      f_aggregate_mapping( 2 * cnt );
    else
      push_int(0);
  } else
    push_int(0);
  if(!DVB->pescnt)
    close(dmx);
}



/**
 * Set a filter for a DVB stream
**/
static int SetFilt(int fd,int pid,int tnr)
{
  struct dmxSctFilterParams FilterParams;
  int ret;

  memset(&FilterParams.filter.filter,0,DMX_FILTER_SIZE);
  memset(&FilterParams.filter.mask,0,DMX_FILTER_SIZE);

  if (tnr >= 0)
  {
    FilterParams.filter.filter[0] = tnr;
    FilterParams.filter.mask[0] = 0xff;
  }
  FilterParams.timeout = 15000;
  FilterParams.flags = DMX_IMMEDIATE_START;
  FilterParams.pid = pid;

  THREADS_ALLOW();
  ret = ioctl(fd,DMX_SET_FILTER,&FilterParams);
  THREADS_DISALLOW();
  if (ret < 0) {
    snprintf (DVB->low_errmsg, MAX_ERR_LEN, "DMX SET SECTION FILTER.\n");
    return 0;
  }
  return 1;
}

/**
 * Stop a PID filter.
**/
static int StopFilt(int fd)
{
  int ret;
  THREADS_ALLOW();
  ret = ioctl(fd,DMX_STOP);
  THREADS_DISALLOW();
  return (!ret);
}


/* FIXME: Remove CRC32 table and use Gz.crc32() */
static unsigned long crc_table[256] =
{
  0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b,
  0x1a864db2, 0x1e475005, 0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
  0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd, 0x4c11db70, 0x48d0c6c7,
  0x4593e01e, 0x4152fda9, 0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
  0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3,
  0x709f7b7a, 0x745e66cd, 0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
  0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5, 0xbe2b5b58, 0xbaea46ef,
  0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
  0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49, 0xc7361b4c, 0xc3f706fb,
  0xceb42022, 0xca753d95, 0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
  0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d, 0x34867077, 0x30476dc0,
  0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
  0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16, 0x018aeb13, 0x054bf6a4,
  0x0808d07d, 0x0cc9cdca, 0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
  0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02, 0x5e9f46bf, 0x5a5e5b08,
  0x571d7dd1, 0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
  0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b, 0xbb60adfc,
  0xb6238b25, 0xb2e29692, 0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
  0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a, 0xe0b41de7, 0xe4750050,
  0xe9362689, 0xedf73b3e, 0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
  0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34,
  0xdc3abded, 0xd8fba05a, 0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
  0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb, 0x4f040d56, 0x4bc510e1,
  0x46863638, 0x42472b8f, 0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
  0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5,
  0x3f9b762c, 0x3b5a6b9b, 0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
  0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623, 0xf12f560e, 0xf5ee4bb9,
  0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
  0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f, 0xc423cd6a, 0xc0e2d0dd,
  0xcda1f604, 0xc960ebb3, 0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
  0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b, 0x9b3660c6, 0x9ff77d71,
  0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
  0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640, 0x4e8ee645, 0x4a4ffbf2,
  0x470cdd2b, 0x43cdc09c, 0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
  0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24, 0x119b4be9, 0x155a565e,
  0x18197087, 0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
  0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d, 0x2056cd3a,
  0x2d15ebe3, 0x29d4f654, 0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
  0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c, 0xe3a1cbc1, 0xe760d676,
  0xea23f0af, 0xeee2ed18, 0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
  0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662,
  0x933eb0bb, 0x97ffad0c, 0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
  0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
};

static unsigned long crc32 (char *data,int len)
{
  unsigned long crc = 0xffffffff;
  int i;

  for (i=0; i<len; i++)
    crc = (crc << 8) ^ crc_table[((crc >> 24) ^ *data++) & 0xff];
  return crc;
}


/* internal */
static int read_t(int fd,unsigned char *buffer,int length,int cks)
{

  struct pollfd u[1];
  int retries;
  int n,l;

  for (retries=0;retries<100;retries++)
  {
    u[0].fd = fd;
    u[0].events = POLLIN;

    n = poll(u,1,20000);
    if (n < 0)
    {
      perror("poll error");
      return -1;
    }
    if (n == 0)
    {
      fprintf(stderr,"timeout\n");
      return -1;
    }

    buffer[0] = 0;

    n = read(fd,buffer+1,length-1);
    if (n < 0)
    {
      perror("read error");
      return -1;
    }

    if (cks && crc32(buffer+1,n) != 0)
    {
      fprintf(stderr,"crc error\n");
      continue;
    }

    break;
  }

  return n+1;
}

/*! @decl mapping analyze_pat()
 *!
 *! Return mapping of all PMT.
 *!
 */
static void f_parse_pat(INT32 args) {

  unsigned char buffer[4096];
  int length,index;
  int n;

  int fd;
  int p, cnt = 0;
  int pid = -1;
  int dmx;

  pop_n_elems(args);
  dmx = open (DEMUXDEVICE, O_RDWR | O_NONBLOCK);
  if (dmx < 0) {
    snprintf (DVB->low_errmsg, MAX_ERR_LEN, "DMX SET SECTION FILTER.\n");
    push_int(0);
    return;
  }

  /* The PAT is supposed to fit in a 184 bytes packet */
  SetFilt(dmx,0,0);
  do
  {
    n = read_t(dmx,buffer,sizeof(buffer),1);
  }
  while (n>=2 && (buffer[0]!=0 || buffer[1]!=0));
  StopFilt(dmx);
  close(dmx);

  if (n < 2)
    push_int(0);

  length = ((buffer[2] & 0x0F) << 8) | buffer[3];
  for (index=9; index<length-4 && index<184; index +=4)
  {
    p = (buffer[index] << 8) | buffer[index+1];
    push_int(p);
    pid = ((buffer[index+2] << 8) | buffer[index+3]) & 0x1FFF;
    push_int(pid);
    cnt++;
  }
  if(cnt)
    f_aggregate_mapping(cnt*2);
  else
    push_int(0);

}

#define SECA_CA_SYSTEM      0x100
#define VIACCESS_CA_SYSTEM  0x500
#define IRDETO_CA_SYSTEM    0x600
#define BETA_CA_SYSTEM     0x1700
#define NAGRA_CA_SYSTEM    0x1800

/**
 * Parse CA descriptor
**/
static void ParseCADescriptor (unsigned char *data,int length)
{
  static char *seca_name = "Seca";
  static char *viaccess_name = "Viaccess";
  static char *nagra_name = "Nagra";
  static char *irdeto_name = "Irdeto";

  int ca_system;
  int j;

  struct ECMINFO *e;

  /*if (debug)
  {
    printf("ca_descr");
    for (j=0; j<length; j++) printf(" %02x",data[j]);
    printf("\n");
  }*/

  /* only test the upper 8 bits of the ca_system */
  ca_system = (data[0] << 8) /* | data[1]*/;

  switch (ca_system)
  {
    case SECA_CA_SYSTEM:
      for (j=2; j<length; j+=15)
      {
        e = malloc(sizeof(struct ECMINFO));
        if (e == 0)
          return;
        e->system = ca_system;
        e->name = seca_name;
        e->ecm_pid = ((data[j] & 0x1f) << 8) | data[j+1];
        e->id = (data[j+2] << 8) | data[j+3];
        e->next = DVB->ecminfo;
        DVB->ecminfo = e;
      }        
      break;
    case VIACCESS_CA_SYSTEM:
      j = 4;
      while (j < length)
      {
        if (data[j]==0x14)
        {
          e = malloc(sizeof(struct ECMINFO));
          if (e == 0)
            return;
          e->system = ca_system;
          e->name = viaccess_name;
          e->ecm_pid = ((data[2] & 0x1f) << 8) | data[3];
          e->id = (data[j+2] << 16) | (data[j+3] << 8) | (data[j+4] & 0xf0);
          e->next = DVB->ecminfo;
          DVB->ecminfo = e;
        }
        j += 2+data[j+1];
      }
      break;
    case IRDETO_CA_SYSTEM:
    case BETA_CA_SYSTEM:
      e = malloc(sizeof(struct ECMINFO));
      if (e == 0)
        return;
      e->system = ca_system;
      e->name = irdeto_name;
      e->ecm_pid = ((data[2] & 0x1f) << 8) | data[3];
      e->next = DVB->ecminfo;
      DVB->ecminfo = e;
//      printf("Found irdeto\n");
      break;
    case NAGRA_CA_SYSTEM:
      e = malloc(sizeof(struct ECMINFO));
      if (e == 0)
        return;
      e->system = ca_system;
      e->name = nagra_name;
      e->ecm_pid = ((data[2] & 0x1f) << 8) | data[3];
      e->next = DVB->ecminfo;
      DVB->ecminfo = e;
//fprintf(stderr,"Found nagra\n");
      break;
  }
}

/**
 * Parse PMT to get ECM PID
**/
/*! @decl void create(int card_number)
 *!
 *! Create a DVB object.
 *!
 *! @param card_number
 *!   The number of card equipment.
 *!
 */
static void f_parse_pmt(INT32 args) 
{
  unsigned char buffer[4096];
  unsigned int length,info_len,data_len, index;
  int retries;
  int pid;
  int n,i;

  int vpid = 0;
  int apid = 0;
  int pnr = -1;

  int program_number;
  int pmt_pid;
  int dmx;

  check_all_args("DVB.dvb->analyze_pmt", args, BIT_INT, BIT_INT, 0);

  dmx = open (DEMUXDEVICE, O_RDWR | O_NONBLOCK);
  if (dmx < 0) {
    snprintf (DVB->low_errmsg, MAX_ERR_LEN, "DMX SET SECTION FILTER.\n");
    push_int(0);
    return;
  }

  pmt_pid = (u_short)Pike_sp[-1].u.integer;
  Pike_sp--;

  program_number = (u_short)Pike_sp[-1].u.integer;
  Pike_sp--;

  SetFilt(dmx,pmt_pid,2);

  for (retries=0; retries<100; retries++)
  {
    do
    {
      n = read_t(dmx,buffer,sizeof(buffer),1);
    }
    while (n>=2 && (buffer[0]!=0 || buffer[1]!=0x02));
   
    length = ((buffer[2] & 0x0F) << 8) | buffer[3];
    if (length+4 > sizeof(buffer))
      n = -1;

    if (n < 2)
      break;

    pnr = (buffer[4]<<8) + buffer[5];
    if (pnr == program_number)
      break;
  }

  StopFilt(dmx);
  close(dmx);

  if (pnr != program_number)
  {
//fprintf(stderr, "Can't find PMT entry\n");
    push_int(0);
    return;
  }

  index = 11;
  info_len = ((buffer[index] & 0x0F) << 8) + buffer[index+1];
  index += 2;

  while (info_len > 0)
  {
    if (buffer[index] == 0x09)
      ParseCADescriptor(&buffer[index+2], buffer[index+1]);

    info_len -= 2+buffer[index+1];
    index += 2+buffer[index+1];
  }

  while (index < length-4)
  {
    pid = ((buffer[index+1] & 0x1f) << 8) + buffer[index+2];

    if (buffer[index]==2 && vpid==0)
      vpid = pid;    
    else
    if ((buffer[index]==3 || buffer[index]==4) && apid==0)
      apid = pid;    

    data_len = ((buffer[index+3] & 0x0F) << 8) + buffer[index+4];
    if (buffer[index]==0x02 || buffer[index]==0x03 || buffer[index]==0x04)
    {
      i = index+5;
      while (i < index+5+data_len)
      {
        switch (buffer[i])
        {
          case 0x0a:
            if (buffer[index]==3 || buffer[index]==4) {
fprintf(stderr, "Language = %.3s\n",&buffer[i+2]);
            }
            break;
          case 0x09:
            ParseCADescriptor(&buffer[i+2],buffer[i+1]);
            break;
        }
        i += 2 + buffer[i+1];
      }
    }
    index += 5 + data_len;
  }

  push_int(1);
}


/*! @decl int audio_mute(int mute)
 *! @decl int audio_mute()
 *!
 *! Mute or unmute audio device.
 *!
 *| @seealso
 *|   @[audio_mixer()]
 */
static void f_audio_mute(INT32 args) {

  int mute = 1; /* default is mute = on */
  int afd, ret;

  check_all_args("DVB.dvb->audio_mute", args, BIT_INT | BIT_VOID, 0);

  if(args) {
    mute = (u_short)Pike_sp[-1].u.integer;
    Pike_sp--;
  }

  afd = open (AUDIODEVICE, O_RDWR);
  if (afd < 0) {
      Pike_error ("opening AUDIO failed\n");
  }
  THREADS_ALLOW();
  ret = ioctl(afd, AUDIO_SET_MUTE, mute);
  THREADS_DISALLOW();
  if(ret < 0)
    push_int(0);
  else
    push_int(1);
  close(afd);

}

/*! @decl mapping audio_status()
 *!
 *! Returns mapping of current audio device status.
 */
static void f_audio_status(INT32 args) {

  int afd, ret;
  audioStatus_t status;

  pop_n_elems(args);
  afd = open (AUDIODEVICE, O_RDWR);
  if (afd < 0) {
      Pike_error ("opening AUDIO failed\n");
  }
  THREADS_ALLOW();
  ret = ioctl(afd, AUDIO_GET_STATUS, &status);
  THREADS_DISALLOW();
  if(ret < 0)
    push_int(0);
  else {
    push_text("av_sync"); push_int(status.AVSyncState);
    push_text("mute"); push_int(status.muteState);
    push_text("state"); 
      switch(status.playState) {
	case AUDIO_STOPPED: push_text("stopped");
			    break;
	case AUDIO_PLAYING: push_text("playing");
			    break;
	case AUDIO_PAUSED: push_text("paused");
			    break;
	default: push_text("unknown");
      }
    push_text("source");
      switch(status.streamSource) {
	case AUDIO_SOURCE_DEMUX: push_text("demux");
			    break;
	case AUDIO_SOURCE_MEMORY: push_text("memory");
			    break;
	default: push_text("unknown");
      }
    push_text("channels");
      switch(status.channelSelect) {
	case AUDIO_STEREO: push_text("stereo");
			    break;
	case AUDIO_MONO_LEFT: push_text("left");
			    break;
	case AUDIO_MONO_RIGHT: push_text("right");
			    break;
	default: push_text("unknown");
      }
    push_text("bypass"); push_int(status.bypassMode);
    f_aggregate_mapping( 2 * 6 );
  }
  
  close(afd);

}

static void f_audio_ctrl(INT32 args) {

  int afd, ret;
  int cw = -1;

  check_all_args("DVB.dvb->ctrl", args, BIT_INT | BIT_STRING, 0);

  if(Pike_sp[-1].type == T_INT)
    cw = (u_short)Pike_sp[-1].u.integer;
  else 
    if(!strcmp(Pike_sp[-1].u.string->str, "play"))
      cw = AUDIO_PLAY;
    else
      if(!strcmp(Pike_sp[-1].u.string->str, "pause"))
        cw = AUDIO_PAUSE;
      else
        if(!strcmp(Pike_sp[-1].u.string->str, "continue"))
          cw = AUDIO_CONTINUE;

  Pike_sp--;
  if(cw == -1) {
    push_int(0);
    return;
  }

  afd = open (AUDIODEVICE, O_RDWR);
  if (afd < 0) {
      Pike_error ("opening AUDIO failed\n");
  }
  THREADS_ALLOW();
  ret = ioctl(afd, cw);
  THREADS_DISALLOW();
  if(ret < 0)
    push_int(0);
  else
    push_int(1);
  close(afd);

}

/*! @decl int audio_mixer(int left, int right)
 *! @decl int audio_mixer(int both)
 *!
 *! Sets output level on DVB audio device.
 *!
 *| @seealso
 *|   @[audio_mute()]
 */
static void f_audio_mixer(INT32 args) {

  int afd, ret;
  audioMixer_t mixer;

  check_all_args("DVB.dvb->audio_mixer", args, BIT_INT, BIT_INT | BIT_VOID, 0);

  mixer.volume_right = (unsigned int)Pike_sp[-1].u.integer;
  Pike_sp--;
  if(args > 1) {
    mixer.volume_left = (unsigned int)Pike_sp[-1].u.integer;
    Pike_sp--;
  } else
    mixer.volume_left = mixer.volume_right;

  afd = open (AUDIODEVICE, O_RDWR);
  if (afd < 0) {
      Pike_error ("opening AUDIO failed\n");
  }
  THREADS_ALLOW();
  ret = ioctl(afd, AUDIO_SET_MIXER, &mixer);
  THREADS_DISALLOW();
  if(ret < 0)
    Pike_error("Seting mixer failed.\n");
  else
    push_int(1);
  close(afd);

}

int find_free_pespos() {

  unsigned int ix;

  for(ix = 0; ix < MAX_PES_FD; ix++)
    if(!DVB->pesfd[ix])
      return ix;
  return -1;
}

int find_pid_pespos(unsigned int pid) {

  unsigned int ix;

  for(ix = 0; ix < MAX_PES_FD; ix++)
    if(DVB->pesid[ix] == pid)
      return ix;
  return -1;
}

/*! @decl int stream_set_buffer(int pid, int len)
 *!
 *! Sets stream's internal buffer.
 *!
 *! @note
 *!   The size is 4096 by default.
 *!
 *| @seealso
 *|   @[stream_attach()]
 */
static void f_stream_set_buffer(INT32 args) {

  int ix, buflen, pid;

  check_all_args("DVB.dvb->stream_set_buffer", args, BIT_INT, BIT_INT, 0);
  buflen = (u_short)Pike_sp[-1].u.integer;
  Pike_sp--;
  pid = (u_short)Pike_sp[-1].u.integer;
  Pike_sp--;
  if((ix = find_pid_pespos(pid)) == -1) {
    push_int(0); /* PID doesn't exist */
    return;
  }
  DVB->pesbuflen[ix] = buflen;
  push_int(1);
}


/*! @decl int stream_attach(int pid,int|function rcb,int ptype)
 *! @decl int stream_attach(int pid,int|function rcb)
 *! @decl int stream_attach(int pid)
 *!
 *! Enable a new stream reader with appropriate PID number.
 *!
 *! @param pid
 *!   PID of stream.
 *!
 *! @param rcb
 *!   Callback function called whenever there is the data to read
 *!   from stream. Only for nonblocking mode.
 *!
 *! @param ptype
 *!   Type of payload data to read. By default, audio data is fetched.
 *!
 *! @note
 *!   Setting async callback doesn't set the object to nonblocking state.
 *!
 *! @seealso
 *!   @[stream_read()], @[stream_detach()]
 */
static void f_stream_attach(INT32 args) {

  struct dmxPesFilterParams pesflt;
  int err, pid, fd, ix, ptype = DMX_PES_OTHER;
  struct svalue feeder;
  unsigned char *pktdata;

  check_all_args("DVB.dvb->stream_attach", args, BIT_INT,
		 BIT_FUNCTION | BIT_INT | BIT_VOID, BIT_INT | BIT_VOID, 0);

  if(DVB->pescnt >= MAX_PES_FD)
      Pike_error("Max opened DEMUX devices reached.\n");

  if(args > 2) {
    ptype = (u_short)Pike_sp[-1].u.integer;
    Pike_sp--;
  }
  if(args > 1) {
#if 0
    feeder = Pike_sp[1-args].u.svalue;
    apply_svalue(&feeder, 0); /* we want more data */
#endif
    Pike_sp--;
  }
  pid = (u_short)Pike_sp[-1].u.integer;
  Pike_sp--;

  if((ix = find_pid_pespos(pid)) != -1) {
    push_int(0); /* PID already attached */
    return;
  }

  fd = open (DEMUXDEVICE, O_RDWR /*| O_NONBLOCK*/);
  if (fd < 0) {
      Pike_error("Opening DEMUX failed.\n");
  }
  pesflt.pid = pid;
  pesflt.input = DMX_IN_FRONTEND;
  pesflt.output = DMX_OUT_TAP;
  pesflt.pesType = ptype;
  pesflt.flags = DMX_IMMEDIATE_START;
#ifdef DVB_DEBUG
  printf("DEB: dvb: set_pes: PID=%d, type=%d\n", pid, ptype);
#endif
  THREADS_ALLOW();
  err = ioctl(fd, DMX_SET_PES_FILTER, &pesflt);
  THREADS_DISALLOW();
  if (err < 0)
    Pike_error("seting PID failed.\n");

  if((pktdata = malloc(DVB->pesbuflen[ix])) == NULL)
    Pike_error("Internal error: can't malloc buffer.\n");
  if(init_p2p(&DVB->p, 2048))
    Pike_error("Internal error: repack size %d is out of range\n", 2048);
  DVB->p.filter = 0xC0; //FIXME: 0xC0 = audio, ...
  DVB->p.es = 1;
  if((ix = find_free_pespos()) == -1)
    Pike_error("Internal error: free position wasn't find.\n");
  DVB->pesfd[ix] = fd;
  DVB->pesid[ix] = pid;
  DVB->pestype[ix] = ptype;
  DVB->pespkt[ix].data = pktdata;
  DVB->pespkt[ix].size = 0;
  /*DVB->pesfcb[ix] = NULL;*/
  DVB->pescnt++;
  push_int(1);

}

/*! @decl int stream_detach(int pid)
 *!
 *! Purge a stream reader.
 *!
 *! @param pid
 *!   PID of stream.
 *!
 *! @seealso
 *!   @[stream_read()], @[stream_attach()]
 */
static void f_stream_detach(INT32 args) {

  unsigned int pid;
  int i;

  check_all_args("DVB.dvb->stream_detach", args, BIT_INT, 0);
  pid = (u_short)Pike_sp[-1].u.integer;
  Pike_sp--;

  if((i = find_pid_pespos(pid)) == -1) {
    push_int(0);
    return;
  }
  
  close(DVB->pesfd[i]);
  DVB->pesid[i] = 0;
  DVB->pesfd[i] = 0;
  DVB->pescnt--;
  free(DVB->pespkt[i].data);
  push_int(1);

}

/*! @decl string|int stream_read(int pid)
 *!
 *! Read data from a stream.
 *!
 *! @param pid
 *!   PID of stream.
 *!
 *! @seealso
 *!   @[stream_attach()], @[stream_detach()]
 */
static void f_stream_read(INT32 args) {

  int i, all = 1, ret, e;
  unsigned int pid, cnt, ix = 0;
  char buf[MAX_DVB_READ_SIZE], *bufptr;

  check_all_args("DVB.dvb->stream_read", args, BIT_INT, BIT_INT | BIT_VOID, 0);
  pid = (u_short)Pike_sp[-1].u.integer;
  Pike_sp--;
  if(args > 1) {
    all = (u_short)Pike_sp[-1].u.integer;
    Pike_sp--;
  }

  if((i = find_pid_pespos(pid)) == -1) {
    push_int(0);
    return;
  }

  do {
    e = 0;
    THREADS_ALLOW();
    ret = read(DVB->pesfd[i], buf + DVB->pespkt[i].size,
	       DVB->pesbuflen[ix] - DVB->pespkt[i].size);
    e=errno; /* check_threads_etc may effect errno */
    THREADS_DISALLOW();

    check_threads_etc();

    if (ret > 0)
      break;
#if 0
    if (ret == -1 && (errno == EAGAIN || errno == EINTR)) {
      buf[0] = '\0';
      ret = 0;
      break;
    }
#endif
  } while(ret < 1); /*while((ret==-1) && (e==EINTR));*/
  if(DVB->pespkt[i].size) {
    memcpy(buf, DVB->pespkt[i].data, DVB->pespkt[i].size);
    ret += DVB->pespkt[i].size;
    DVB->pespkt[i].size = 0;
  }

  if(ret > 0) {
    bufptr = buf;
    while((cnt = get_pes_filt(bufptr,ret,&DVB->p, &DVB->pespkt[i])) > 0) {
      push_string(make_shared_binary_string((char *)DVB->pespkt[i].data,
		    DVB->pespkt[i].size));
      ix++;
      bufptr += cnt;
      ret -= cnt;
    }
    if(ix)
      f_add(ix);
    if(ret) {
       /* some unprocessed data remain in buf */
       DVB->pespkt[i].size = ret;
    }
    return;
  }
  push_int(0);

}

static void f_stream_info(INT32 args) {

  check_all_args("DVB.dvb->stream_info", args, BIT_INT, 0);
  pop_n_elems(args);
  push_int(0);

}

static void f__sprintf(INT32 args) {

  unsigned int n = 0, x, ix, cnt;

  check_all_args("DVB.dvb->_sprintf", args, BIT_INT, BIT_MAPPING | BIT_VOID, 0);

  x = Pike_sp[-args].u.integer;
  pop_n_elems(args);
  switch (x) {
    case 'O':
            n++; push_text("DVB.dvb(");
            n++; push_text(DEMUXDEVICE);
	    n++; push_text(": ");
	    cnt = 0;
	    for(ix=0; ix < MAX_PES_FD; ix++) {
	      if(!DVB->pesfd[ix])
		continue;
	      n++; push_int(DVB->pesid[ix]);
	      n++; push_text("/");
	      n++; switch(DVB->pestype[ix]) {
		     case DMX_PES_AUDIO: push_text("a"); break;
		     case DMX_PES_VIDEO: push_text("v"); break;
		     case DMX_PES_TELETEXT: push_text("t"); break;
		     case DMX_PES_SUBTITLE: push_text("s"); break;
		     case DMX_PES_OTHER: push_text("o"); break;
		     default: push_text("?");
		   }
	      cnt++;
	      if(cnt < DVB->pescnt) {
		n++; push_text(",");
	      }
	    }
            n++; push_text(")");
            f_add(n);
            return;
     default:
            push_int(0);
            return;
    }
}



/*! @endclass
 */

/*! @endmodule
 */

static void init_dvb_data(struct object *obj) {

  unsigned int i;

  DVB->cardn = -1;
  for(i = 0; i < MAX_PES_FD; i++) {
    DVB->pesfd[i] = 0;
    DVB->pesid[i] = 0;
    DVB->pespkt[i].data = NULL;
    DVB->pesbuflen[i] = MAX_DVB_READ_SIZE;
  }
  DVB->pescnt = 0;
  DVB->ecminfo = NULL;
  memset(&DVB->low_errmsg, '\0', sizeof(DVB->low_errmsg));
}

static void exit_dvb_data(struct object *obj) {

  struct ECMINFO *e;
  unsigned int i;

  if(DVB->cardn != -1) {
    close(DVB->fefd);
    for(i = 0; i < MAX_PES_FD; i++) {
      if(DVB->pesfd)
	close(DVB->pesfd[i]);
      if(DVB->pespkt[i].data != NULL)
        free(DVB->pespkt[i].data);
    }
  }
  if(DVB->ecminfo != NULL)
    do {
      e = DVB->ecminfo->next;
      free(DVB->ecminfo);
      DVB->ecminfo = e;
    } while (e != NULL);
}

/*
 * ---------------------
 *    Pike module API
 * ---------------------
 */

void pike_module_init() {

  /*
   * Internal constant names
   *
   */
  add_integer_constant("MUX_AUDIO", DMX_PES_AUDIO, 0);
  add_integer_constant("MUX_VIDEO", DMX_PES_VIDEO, 0);
  add_integer_constant("MUX_TELETEXT", DMX_PES_TELETEXT, 0);
  add_integer_constant("MUX_SUBTITLE", DMX_PES_SUBTITLE, 0);
  add_integer_constant("MUX_PCR", DMX_PES_PCR, 0);
  add_integer_constant("MUX_OTHER", DMX_PES_OTHER, 0);

  add_integer_constant("CA_SYSTEM_SECA", SECA_CA_SYSTEM, 0);
  add_integer_constant("CA_SYSTEM_VIACCESS", VIACCESS_CA_SYSTEM, 0);
  add_integer_constant("CA_SYSTEM_IRDETO", IRDETO_CA_SYSTEM, 0);
  add_integer_constant("CA_SYSTEM_BETA", BETA_CA_SYSTEM, 0);
  add_integer_constant("CA_SYSTEM_NAGRA", NAGRA_CA_SYSTEM, 0);

  start_new_program();
  ADD_STORAGE(dvb_data);
  set_init_callback(init_dvb_data);
  set_exit_callback(exit_dvb_data);

  /* dvb */
  add_function("create", f_create, "function(int|void:void)", 0);
  add_function("_sprintf", f__sprintf, "function(int,mapping|void:mixed)", 0);
  add_function("tune", f_zap, "function(int,int,int|string,int,mapping|void:int)", 0);
  //add_function("set_pids", f_set_pids, "function(string,mixed:int)", 0);
  add_function("get_pids", f_get_pids, "function(:mapping|int)", 0);
  add_function("analyze_pat", f_parse_pat, "function(:mapping|int)", 0);
  add_function("analyze_pmt", f_parse_pmt, "function(int,int:array|int)", 0);
  /*end_class("dvb", 0); */

  /* Frontend */
  add_function("fe_status", f_fe_status, "function(:mapping|int)", 0);
  add_function("fe_info", f_fe_info, "function(:mapping|int)", 0);

  /* Audio */
  add_function("audio_mute", f_audio_mute, "function(int|void:int)", 0);
  add_function("audio_status", f_audio_status, "function(:mapping|int)", 0);
  add_function("audio_ctrl", f_audio_ctrl, "function(int|string:int)", 0);
  add_function("audio_mixer", f_audio_mixer, "function(int,int|void:int)", 0);

  /* PES streams */
  add_function("stream_attach", f_stream_attach, "function(int,function|int|void,int|void:int)", 0);
  add_function("stream_detach", f_stream_detach, "function(int:int)", 0);
  add_function("stream_read", f_stream_read, "function(int,int|void:string|int)", 0);
  add_function("stream_set_buffer", f_stream_set_buffer, "function(int,int:int)", 0);
  add_function("stream_info", f_stream_info, "function(int:mapping|int)", 0);

  end_class("dvb", 0);

} /* pike_module_init */

void pike_module_exit() {

  if(dvb_program) {
    free_program(dvb_program);
    dvb_program = NULL;
  }
} /* pike_module_exit */

#else

void pike_module_init() {

  add_integer_constant("DVB support IS MISSING", 0, 0);
}

void pike_module_exit() {
}

#endif
