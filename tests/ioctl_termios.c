/*
 * Check decoding of struct termio{,s,s2}-related commands of ioctl syscall.
 *
 * Copyright (c) 2018 The strace developers.
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "tests.h"

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <linux/ioctl.h>
#include <linux/termios.h>
#include <linux/tty.h>

#include <sys/param.h>

#include "xlat.h"
#include "xlat/baud_options.h"
#include "xlat/term_line_discs.h"

#ifndef VERBOSE
# define VERBOSE 0
#endif

#define PRINT_FLAG(val_, f_) \
	do { \
		if ((val_ & f_)) { \
			printf("%s%s", sep, #f_); \
			val_ &= ~f_; \
			sep = "|"; \
		} \
	} while (0)

extern int ioctl (int __fd, unsigned long int __request, ...);

static void
print_flags(tcflag_t iflag, tcflag_t oflag, tcflag_t cflag, tcflag_t lflag)
{
	const char *sep = "";

	printf("c_iflag=");
	PRINT_FLAG(iflag, IGNBRK);
	PRINT_FLAG(iflag, BRKINT);
	PRINT_FLAG(iflag, IGNPAR);
	PRINT_FLAG(iflag, PARMRK);
	PRINT_FLAG(iflag, INPCK);
	PRINT_FLAG(iflag, ISTRIP);
	PRINT_FLAG(iflag, INLCR);
	PRINT_FLAG(iflag, IGNCR);
	PRINT_FLAG(iflag, ICRNL);
	PRINT_FLAG(iflag, IUCLC);
	PRINT_FLAG(iflag, IXON);
	PRINT_FLAG(iflag, IXANY);
	PRINT_FLAG(iflag, IXOFF);
	PRINT_FLAG(iflag, IMAXBEL);
	PRINT_FLAG(iflag, IUTF8);
	if (iflag)
		printf("%s%#x", sep, iflag);

	/* oflag */
	static struct {
		tcflag_t val;
		const char *prefix;
		unsigned int max_val;
	} vals[] = {
		{ NLDLY,  "NL",  1 },
		{ CRDLY,  "CR",  3 },
		{ TABDLY, "TAB", 3 },
		{ BSDLY,  "BS",  1 },
		{ VTDLY,  "VT",  1 },
		{ FFDLY,  "FF",  1 },
	};

	printf(", c_oflag=");

	for (unsigned int i = 0; i < ARRAY_SIZE(vals); i++) {
		int val = (oflag & vals[i].val) /
			(vals[i].val / vals[i].max_val);
#ifndef __alpha__
		if (i == 2 && val == 3) /* XTABS */
			printf("XTABS|");
		else
#endif
			printf("%s%u|", vals[i].prefix, val);
		oflag &= ~vals[i].val;
	}

	sep = "";
#if defined __alpha__ && defined XTABS
	PRINT_FLAG(oflag, XTABS);
#endif
	PRINT_FLAG(oflag, OPOST);
	PRINT_FLAG(oflag, OLCUC);
	PRINT_FLAG(oflag, ONLCR);
	PRINT_FLAG(oflag, OCRNL);
	PRINT_FLAG(oflag, ONOCR);
	PRINT_FLAG(oflag, ONLRET);
	PRINT_FLAG(oflag, OFILL);
	PRINT_FLAG(oflag, OFDEL);
#ifdef PAGEOUT
	PRINT_FLAG(oflag, PAGEOUT);
#endif
#ifdef WRAP
	PRINT_FLAG(oflag, WRAP);
#endif
	if (oflag)
		printf("%s%#x", sep, oflag);

	/* cflag */
	sep = "";
	printf(", c_cflag=");
	printxval(baud_options, cflag & CBAUD, "B???");
	printf("|");
	if (cflag & CIBAUD) {
		printxval(baud_options, (cflag & CIBAUD) >> IBSHIFT, "B???");
		printf("<<IBSHIFT|");
	}
	switch (cflag & CSIZE) {
	case CS5:
		printf("CS5|");
		break;
	case CS6:
		printf("CS6|");
		break;
	case CS7:
		printf("CS7|");
		break;
	case CS8:
		printf("CS8|");
		break;
	}
	cflag &= ~(CBAUD | CIBAUD | CSIZE);

	PRINT_FLAG(cflag, CSTOPB);
	PRINT_FLAG(cflag, CREAD);
	PRINT_FLAG(cflag, PARENB);
	PRINT_FLAG(cflag, PARODD);
	PRINT_FLAG(cflag, HUPCL);
	PRINT_FLAG(cflag, CLOCAL);
#ifdef CTVB
	PRINT_FLAG(cflag, CTVB);
#endif
#ifdef CMSPAR
	PRINT_FLAG(cflag, CMSPAR);
#endif
#ifdef CRTSCTS
	PRINT_FLAG(cflag, CRTSCTS);
#endif
	if (cflag)
		printf("%s%#x", sep, cflag);

	/* lflag */
	sep = "";
	printf(", c_lflag=");
	PRINT_FLAG(lflag, ISIG);
	PRINT_FLAG(lflag, ICANON);
	PRINT_FLAG(lflag, XCASE);
	PRINT_FLAG(lflag, ECHO);
	PRINT_FLAG(lflag, ECHOE);
	PRINT_FLAG(lflag, ECHOK);
	PRINT_FLAG(lflag, ECHONL);
	PRINT_FLAG(lflag, NOFLSH);
	PRINT_FLAG(lflag, IEXTEN);
	PRINT_FLAG(lflag, ECHOCTL);
	PRINT_FLAG(lflag, ECHOPRT);
	PRINT_FLAG(lflag, ECHOKE);
	PRINT_FLAG(lflag, FLUSHO);
	PRINT_FLAG(lflag, PENDIN);
	PRINT_FLAG(lflag, TOSTOP);
	PRINT_FLAG(lflag, EXTPROC);
#ifdef DEFECHO
	PRINT_FLAG(lflag, DEFECHO);
#endif
	if (lflag)
		printf("%s%#x", sep, lflag);
}

#define cc_def_(cc_) \
	[cc_] = #cc_

#if VERBOSE
static void
print_termios_cc(const cc_t *ccs, size_t size, bool tios)
{
	static const char * const cc_tio_names[] = {
#if defined __alpha__ || defined __powerpc__
		cc_def_(_VINTR),
		cc_def_(_VQUIT),
		cc_def_(_VERASE),
		cc_def_(_VKILL),
		cc_def_(_VEOF),
		cc_def_(_VMIN),
		cc_def_(_VEOL),
		cc_def_(_VTIME),
		cc_def_(_VEOL2),
		cc_def_(_VSWTC),
#endif
	};

	static const char * const cc_tios_names[] = {
		cc_def_(VINTR),
		cc_def_(VQUIT),
		cc_def_(VERASE),
		cc_def_(VKILL),
		cc_def_(VEOL2),
		cc_def_(VSWTC),
		cc_def_(VSTART),
		cc_def_(VSTOP),
		cc_def_(VSUSP),
		cc_def_(VREPRINT),
		cc_def_(VDISCARD),
		cc_def_(VWERASE),
		cc_def_(VLNEXT),
		cc_def_(VEOF),
		cc_def_(VEOL),
#ifdef VDSUSP
		cc_def_(VDSUSP),
#endif
#if VMIN != VEOF
		cc_def_(VMIN),
#endif
#if VTIME != VEOL
		cc_def_(VTIME),
#endif
	};

	printf("c_cc=[");

	for (size_t i = 0; i < size; i++) {
		bool has_name = tios ?
			(i < ARRAY_SIZE(cc_tios_names)) && cc_tios_names[i] :
#if __alpha__
			(i < ARRAY_SIZE(cc_tio_names)) && cc_tio_names[i];
#else
			false;
#endif
		const char *name = has_name ?
			(tios ? cc_tios_names : cc_tio_names)[i] : "";

		printf("%s[%s%.0zu] = %#hhx",
		       i ? ", " : "", name, has_name ? 0 : i, ccs[i]);
	}

	printf("]");
}
#endif /* VERBOSE */

#ifdef HAVE_STRUCT_TERMIOS2
static void
print_termios2(void *tios_ptr)
{
	struct termios2 *tios = tios_ptr;

	printf("{");
	print_flags(tios->c_iflag, tios->c_oflag, tios->c_cflag, tios->c_lflag);
	printf(", ");

#if VERBOSE
	printf("c_line=");
	printxval(term_line_discs, tios->c_line, "N_???");
	printf(", ");

	if (!(tios->c_lflag & ICANON))
		printf("c_cc[VMIN]=%hhu, c_cc[VTIME]=%u, ",
		       tios->c_cc[VMIN], tios->c_cc[VTIME]);

	print_termios_cc(tios->c_cc, sizeof(tios->c_cc), true);

	printf(", c_ispeed=%u, c_ospeed=%u", tios->c_ispeed, tios->c_ospeed);
#else /* !VERBOSE */
	printf("...");
#endif /* VERBOSE */

	printf("}");
}
#endif

static void
print_termios(void *tios_ptr)
{
	struct termios *tios = tios_ptr;

	printf("{");
	print_flags(tios->c_iflag, tios->c_oflag, tios->c_cflag, tios->c_lflag);
	printf(", ");

#if VERBOSE
	printf("c_line=");
	printxval(term_line_discs, tios->c_line, "N_???");
	printf(", ");

	if (!(tios->c_lflag & ICANON))
		printf("c_cc[VMIN]=%hhu, c_cc[VTIME]=%u, ",
		       tios->c_cc[VMIN], tios->c_cc[VTIME]);

	print_termios_cc(tios->c_cc, sizeof(tios->c_cc), true);

# if HAVE_STRUCT_TERMIOS_C_ISPEED
	printf(", c_ispeed=%u", tios->c_ispeed);
# endif
# if HAVE_STRUCT_TERMIOS_C_OSPEED
	printf(", c_ospeed=%u", tios->c_ospeed);
# endif
#else /* !VERBOSE */
	printf("...");
#endif /* VERBOSE */

	printf("}");
}

static void
print_termio(void *tios_ptr)
{
	struct termio *tios = tios_ptr;

#if VERBOSE
# if defined __alpha__ || defined __powerpc__
	const bool alpha = true;
	const unsigned vmin = _VMIN;
	const unsigned vtime = _VTIME;
# else
	const bool alpha = false;
	const unsigned vmin = VMIN;
	const unsigned vtime = VTIME;
# endif
#endif /* VERBOSE */

	printf("{");
	print_flags(tios->c_iflag, tios->c_oflag, tios->c_cflag, tios->c_lflag);

	printf(", ");

#if VERBOSE
	printf("c_line=");
	printxval(term_line_discs, tios->c_line, "N_???");
	printf(", ");

	if (!(tios->c_lflag & ICANON))
		printf("c_cc[%sVMIN]=%hhu, c_cc[%sVTIME]=%u, ",
		       alpha ? "_" : "", tios->c_cc[vmin],
		       alpha ? "_" : "", tios->c_cc[vtime]);

	print_termios_cc(tios->c_cc, sizeof(tios->c_cc), !alpha);
#else /* !VERBOSE */
	printf("...");
#endif /* VERBOSE */

	printf("}");
}

static void
do_ioctl(kernel_ulong_t cmd, const char *cmd_str, int fd,
	 void (*printer)(void *data), kernel_ulong_t data_ptr, bool valid,
	 bool write, const char *data_str, bool can_fail)
{
	long ret = 0;
	long saved_errno;
	void *data = (void *) (uintptr_t) data_ptr;

	if (!write) {
		ret = ioctl(fd, cmd, data_ptr);
		saved_errno = errno;
	}

	printf("ioctl(%d, %s, ", fd, cmd_str);

	if (valid && !ret) {
		if (data_str)
			printf("%s", data_str);
		else
			printer(data);
	} else {
		if (data)
			printf("%#llx", (unsigned long long) data_ptr);
		else
			printf("NULL");
	}

	if (write) {
		ret = ioctl(fd, cmd, data_ptr);

		if (valid && ret && !can_fail)
			perror_msg_and_fail("ioctl(%d, %#llx, %#llx) = -1",
					    fd, (unsigned long long) cmd,
					    (unsigned long long) data_ptr);
	} else {
		errno = saved_errno;
	}

	printf(") = %s\n", sprintrc(ret));

}

#ifdef HAVE_STRUCT_TERMIOS2
static const char *
setup_termios2(void *tios_ptr, int variant)
{
	struct termios2 *tios = tios_ptr;

	switch (variant) {
	case 0:
		fill_memory(tios, sizeof(*tios));
		return NULL;

	case 1:
		fill_memory_ex(tios, sizeof(*tios), 0xA5, 0x5A);
		return NULL;

	case 2:
		memset(tios, 0, sizeof(*tios));

		tios->c_iflag = IGNBRK|IUTF8|0xdead0000;
		tios->c_oflag = NL0|CR2|TAB3|BS0|VT1|FF0|
#ifdef __alpha__
				XTABS|
#endif
				OPOST|ONLCR|OFILL|
#ifdef PAGEOUT
				PAGEOUT|
#endif
				0xbad00000;
		tios->c_cflag = B75|(B57600<<IBSHIFT)|CS6|CSTOPB|
#ifdef CTVB
				CTVB|
#endif
#ifdef CMSPAR
				CMSPAR|
#endif
				0;
		tios->c_lflag = ISIG|ECHOE|FLUSHO|
#ifdef DEFECHO
				DEFECHO|
#endif
				0xfee00000;

		tios->c_line = N_IRDA;

		tios->c_cc[VTIME] = 160;
		tios->c_cc[VMIN] = 137;
		tios->c_cc[VLNEXT] = 0xff;
		tios->c_cc[VSWTC] = 0x2a;

		tios->c_ispeed = 3141592653U;
		tios->c_ospeed = 2718281828U;

		return "{c_iflag=IGNBRK|IUTF8|0xdead0000, "
		       "c_oflag=NL0|CR2|"
#ifdef __alpha__
		       "TAB3"
#else
		       "XTABS"
#endif
		       "|BS0|VT1|FF0|"
#ifdef __alpha__
		       "XTABS|"
#endif
		       "OPOST|ONLCR|OFILL|"
#ifdef PAGEOUT
		       "PAGEOUT|"
#endif
		       "0xbad00000, "
		       "c_cflag=B75|B57600<<IBSHIFT|CS6|CSTOPB"
#ifdef CTVB
		       "|CTVB"
#endif
#ifdef CMSPAR
		       "|CMSPAR"
#endif
		       ", "
		       "c_lflag=ISIG|ECHOE|FLUSHO|"
#ifdef DEFECHO
		       "DEFECHO|"
#endif
		       "0xfee00000, "
#if VERBOSE
		       "c_line=N_IRDA, "
		       "c_cc[VMIN]=137, "
		       "c_cc[VTIME]=160, "
# if defined __alpha__
		       "c_cc=[[VEOF] = 0, [VEOL] = 0, [VEOL2] = 0, "
		       "[VERASE] = 0, [VWERASE] = 0, [VKILL] = 0, "
		       "[VREPRINT] = 0x89, [VSWTC] = 0x2a, [VINTR] = 0, "
		       "[VQUIT] = 0, [VSUSP] = 0, [VSTART] = 0, [VSTOP] = 0, "
		       "[VLNEXT] = 0xff, [VDISCARD] = 0, [VMIN] = 0x89, "
		       "[VTIME] = 0xa0, [17] = 0, [18] = 0]"
# elif defined __mips__
		       "c_cc=[[VINTR] = 0, [VQUIT] = 0, [VERASE] = 0, "
		       "[VKILL] = 0, [VEOF] = 0, [VTIME] = 0xa0, "
		       "[VMIN] = 0x89, [VSWTC] = 0x2a, [VSTART] = 0, "
		       "[VSTOP] = 0, [VSUSP] = 0, [VDSUSP] = 0, "
		       "[VREPRINT] = 0, [VDISCARD] = 0, [VWERASE] = 0, "
		       "[VLNEXT] = 0xff, [VEOL2] = 0, [V_EOL] = 0, [18] = 0, "
		       "[19] = 0, [20] = 0, [21] = 0, [22] = 0]"
# elif defined __sparc__
		       "c_cc=[[VINTR] = 0, [VQUIT] = 0, [VERASE] = 0, "
		       "[VKILL] = 0, [VEOF] = 0x89, [VEOL] = 0xa0, "
		       "[VEOL2] = 0, [VSWTC] = 0x2a, [VSTART] = 0, "
		       "[VSTOP] = 0, [VSUSP] = 0, [VDSUSP] = 0, "
		       "[VREPRINT] = 0, [VDISCARD] = 0, [VWERASE] = 0, "
		       "[VLNEXT] = 0xff, [16] = 0, [17] = 0, [18] = 0]"
# else
		       "c_cc=[[VINTR] = 0, [VQUIT] = 0, [VERASE] = 0, "
		       "[VKILL] = 0, [VEOF] = 0, [VTIME] = 0xa0, "
		       "[VMIN] = 0x89, [VSWTC] = 0x2a, [VSTART] = 0, "
		       "[VSTOP] = 0, [VSUSP] = 0, [VEOL] = 0, [VREPRINT] = 0, "
		       "[VDISCARD] = 0, [VWERASE] = 0, [VLNEXT] = 0xff, "
		       "[VEOL2] = 0, [17] = 0, [18] = 0]"
#endif
		       ", c_ispeed=3141592653, c_ospeed=2718281828"
#else /* !VERBOSE */
		       "..."
#endif /* VERBOSE */
		       "}";
	}

	return NULL;
}
#endif

static const char *
setup_termios(void *tios_ptr, int variant)
{
	struct termios *tios = tios_ptr;

	switch (variant) {
	case 0:
		fill_memory(tios, sizeof(*tios));
		return NULL;

	case 1:
		fill_memory_ex(tios, sizeof(*tios), 0xA5, 0x5A);
		return NULL;

	case 2:
		memset(tios, 0, sizeof(*tios));

		tios->c_iflag = IGNBRK|IUTF8|0xdead0000;
		tios->c_oflag = NL0|CR2|TAB3|BS0|VT1|FF0|
#ifdef __alpha__
				XTABS|
#endif
				OPOST|ONLCR|OFILL|
#ifdef PAGEOUT
				PAGEOUT|
#endif
				0xbad00000;
		tios->c_cflag = B75|(B57600<<IBSHIFT)|CS6|CSTOPB|
#ifdef CTVB
				CTVB|
#endif
#ifdef CMSPAR
				CMSPAR|
#endif
				0;
		tios->c_lflag = ISIG|ECHOE|FLUSHO|
#ifdef DEFECHO
				DEFECHO|
#endif
				0xfee00000;

		tios->c_line = N_AX25;

		tios->c_cc[VTIME] = 160;
		tios->c_cc[VMIN] = 137;
		tios->c_cc[VLNEXT] = 0xff;
		tios->c_cc[VSWTC] = 0x2a;

# if HAVE_STRUCT_TERMIOS_C_ISPEED
		tios->c_ispeed = 3141592653U;
# endif
# if HAVE_STRUCT_TERMIOS_C_OSPEED
		tios->c_ospeed = 2718281828U;
# endif

		return "{c_iflag=IGNBRK|IUTF8|0xdead0000, "
		       "c_oflag=NL0|CR2|"
#ifdef __alpha__
		       "TAB3"
#else
		       "XTABS"
#endif
		       "|BS0|VT1|FF0|"
#ifdef __alpha__
		       "XTABS|"
#endif
		       "OPOST|ONLCR|OFILL|"
#ifdef PAGEOUT
		       "PAGEOUT|"
#endif
		       "0xbad00000, "
		       "c_cflag=B75|B57600<<IBSHIFT|CS6|CSTOPB"
#ifdef CTVB
		       "|CTVB"
#endif
#ifdef CMSPAR
		       "|CMSPAR"
#endif
		       ", "
		       "c_lflag=ISIG|ECHOE|FLUSHO|"
#ifdef DEFECHO
		       "DEFECHO|"
#endif
		       "0xfee00000, "
#if VERBOSE
		       "c_line=N_AX25, "
		       "c_cc[VMIN]=137, "
		       "c_cc[VTIME]=160, "
# if defined __alpha__
		       "c_cc=[[VEOF] = 0, [VEOL] = 0, [VEOL2] = 0, "
		       "[VERASE] = 0, [VWERASE] = 0, [VKILL] = 0, "
		       "[VREPRINT] = 0x89, [VSWTC] = 0x2a, [VINTR] = 0, "
		       "[VQUIT] = 0, [VSUSP] = 0, [VSTART] = 0, [VSTOP] = 0, "
		       "[VLNEXT] = 0xff, [VDISCARD] = 0, [VMIN] = 0x89, "
		       "[VTIME] = 0xa0, [17] = 0, [18] = 0]"
# elif defined __mips__
		       "c_cc=[[VINTR] = 0, [VQUIT] = 0, [VERASE] = 0, "
		       "[VKILL] = 0, [VEOF] = 0, [VTIME] = 0xa0, "
		       "[VMIN] = 0x89, [VSWTC] = 0x2a, [VSTART] = 0, "
		       "[VSTOP] = 0, [VSUSP] = 0, [VDSUSP] = 0, "
		       "[VREPRINT] = 0, [VDISCARD] = 0, [VWERASE] = 0, "
		       "[VLNEXT] = 0xff, [VEOL2] = 0, [V_EOL] = 0, [18] = 0, "
		       "[19] = 0, [20] = 0, [21] = 0, [22] = 0]"
# elif defined __powerpc__
		       "c_cc=[[VINTR] = 0, [VQUIT] = 0, [VERASE] = 0, "
		       "[VKILL] = 0, [VEOF] = 0, [VMIN] = 0x89, "
		       "[VEOL] = 0, [VTIME] = 0xa0, [VEOL2] = 0, "
		       "[VSWTC] = 0x2a, [VWERASE] = 0, [VREPRINT] = 0, "
		       "[VSUSP] = 0, [VSTART] = 0, [VSTOP] = 0, "
		       "[VLNEXT] = 0xff, [VDISCARD] = 0, [17] = 0, [18] = 0]"
# elif defined __sparc__
		       "c_cc=[[VINTR] = 0, [VQUIT] = 0, [VERASE] = 0, "
		       "[VKILL] = 0, [VEOF] = 0x89, [VEOL] = 0xa0, "
		       "[VEOL2] = 0, [VSWTC] = 0x2a, [VSTART] = 0, "
		       "[VSTOP] = 0, [VSUSP] = 0, [VDSUSP] = 0, "
		       "[VREPRINT] = 0, [VDISCARD] = 0, [VWERASE] = 0, "
		       "[VLNEXT] = 0xff, [16] = 0]"
# else
		       "c_cc=[[VINTR] = 0, [VQUIT] = 0, [VERASE] = 0, "
		       "[VKILL] = 0, [VEOF] = 0, [VTIME] = 0xa0, "
		       "[VMIN] = 0x89, [VSWTC] = 0x2a, [VSTART] = 0, "
		       "[VSTOP] = 0, [VSUSP] = 0, [VEOL] = 0, [VREPRINT] = 0, "
		       "[VDISCARD] = 0, [VWERASE] = 0, [VLNEXT] = 0xff, "
		       "[VEOL2] = 0, [17] = 0, [18] = 0]"
#endif
# if HAVE_STRUCT_TERMIOS_C_ISPEED
		       ", c_ispeed=3141592653"
# endif
# if HAVE_STRUCT_TERMIOS_C_OSPEED
		       ", c_ospeed=2718281828"
# endif
#else /* !VERBOSE */
		       "..."
#endif /* VERBOSE */
		       "}";
	}

	return NULL;
}

static const char *
setup_termio(void *tios_ptr, int variant)
{
	struct termio *tios = tios_ptr;

	switch (variant) {
	case 0:
		fill_memory(tios, sizeof(*tios));
		return NULL;

	case 1:
		fill_memory_ex(tios, sizeof(*tios), 0xA5, 0x5A);
		return NULL;

	case 2:
		memset(tios, 0, sizeof(*tios));

		tios->c_iflag = IGNBRK|IUTF8;
		tios->c_oflag = NL0|CR2|TAB3|BS0|VT1|FF0|
#ifdef __alpha__
				XTABS|
#endif
				OPOST|ONLCR|OFILL|
#ifdef PAGEOUT
				PAGEOUT|
#endif
				0;
		tios->c_cflag = B75|CS6|CSTOPB;
		tios->c_lflag = ISIG|ECHOE|FLUSHO|
#ifdef DEFECHO
				DEFECHO|
#endif
				0;

		tios->c_line = 234;

#if defined __alpha__ || defined __powerpc__
		tios->c_cc[_VTIME] = 160;
		tios->c_cc[_VMIN] = 137;
		tios->c_cc[_VSWTC] = 0x2a;
#else
		tios->c_cc[VTIME] = 160;
		tios->c_cc[VMIN] = 137;
		tios->c_cc[VSWTC] = 0x2a;
#endif

		return "{c_iflag=IGNBRK|IUTF8, "
		       "c_oflag=NL0|CR2|"
#ifdef __alpha__
		       "TAB3"
#else
		       "XTABS"
#endif
		       "|BS0|VT1|FF0"
#ifdef __alpha__
		       "|XTABS"
#endif
		       "|OPOST|ONLCR|OFILL"
#ifdef PAGEOUT
		       "|PAGEOUT"
#endif
		       ", "
		       "c_cflag=B75|CS6|CSTOPB, "
		       "c_lflag=ISIG|ECHOE|FLUSHO"
#ifdef DEFECHO
		       "|DEFECHO"
#endif
		       ", "
#if VERBOSE
		       "c_line=0xea /* N_??? */, "
# if defined __alpha__
		       "c_cc[_VMIN]=137, "
		       "c_cc[_VTIME]=160, "
		       "c_cc=[[_VEOF] = 0, [_VEOL] = 0, [_VEOL2] = 0, "
		       "[_VERASE] = 0, [_VWERASE] = 0, [_VKILL] = 0, "
		       "[_VREPRINT] = 0x89, [_VSWTC] = 0x2a]"
# elif defined __mips__
		       "c_cc[VMIN]=137, "
		       "c_cc[VTIME]=160, "
		       "c_cc=[[VINTR] = 0, [VQUIT] = 0, [VERASE] = 0, "
		       "[VKILL] = 0, [VEOF] = 0, [VTIME] = 0xa0, "
		       "[VMIN] = 0x89, [VSWTC] = 0x2a]"
# elif defined __powerpc__
		       "c_cc[_VMIN]=137, "
		       "c_cc[_VTIME]=160, "
		       "c_cc=[[_VINTR] = 0, [_VQUIT] = 0, [_VERASE] = 0, "
		       "[_VKILL] = 0, [_VEOF] = 0, [_VMIN] = 0x89, "
		       "[_VEOL] = 0, [_VTIME] = 0xa0, [_VEOL2] = 0, "
		       "[_VSWTC] = 0x2a]"
# elif defined __sparc__
		       "c_cc[VMIN]=137, "
		       "c_cc[VTIME]=160, "
		       "c_cc=[[VINTR] = 0, [VQUIT] = 0, [VERASE] = 0, "
		       "[VKILL] = 0, [VEOF] = 0x89, [VEOL] = 0xa0, "
		       "[VEOL2] = 0, [VSWTC] = 0x2a]"
# else
		       "c_cc[VMIN]=137, "
		       "c_cc[VTIME]=160, "
		       "c_cc=[[VINTR] = 0, [VQUIT] = 0, [VERASE] = 0, "
		       "[VKILL] = 0, [VEOF] = 0, [VTIME] = 0xa0, "
		       "[VMIN] = 0x89, [VSWTC] = 0x2a]"
#endif
#else /* !VERBOSE */
		       "..."
#endif
		       "}";
	}

	return NULL;
}

int
main(void)
{
	int ret;

	struct termio *tio = tail_alloc(sizeof(*tio));
	struct termios *tios1 = tail_alloc(sizeof(*tios1));
#ifdef HAVE_STRUCT_TERMIOS2
	struct termios2 *tios2 = tail_alloc(sizeof(*tios2));
#endif

	struct {
		struct {
			kernel_ulong_t cmd;
			const char *cmd_str;
			bool write;
			bool can_fail;
		} cmds[6];
		struct {
			kernel_ulong_t data;
			const char *data_str;
			bool valid;
		} args[4]; /* The last one should be valid */
		void (*printer)(void *data);
		const char * (*setup)(void *data, int variant);
		unsigned int setup_variants;
	} checks[] = {
#ifdef HAVE_STRUCT_TERMIOS2
		{
			{
				{ ARG_STR(TCSETS2),  true },
				{ ARG_STR(TCSETSW2), true },
				{ ARG_STR(TCSETSF2), true },
				{ ARG_STR(TCGETS2),  false },
			},
			{
				{ (uintptr_t) ARG_STR(NULL), false },
				{ (uintptr_t) (tios2 + 1), NULL, false },
				{ (uintptr_t) tios2 + 4, NULL, false },
				{ (uintptr_t) tios2, NULL, true },
			},
			print_termios2, setup_termios2, 3
		},
#endif
		{
			{
				/* XXX */
				{ TCSETS,  "SNDCTL_TMR_START or TCSETS", true },
				{ TCSETSW, "SNDCTL_TMR_STOP or TCSETSW", true },
				{ TCSETSF, "SNDCTL_TMR_CONTINUE or TCSETSF",
					true },

				{ ARG_STR(TCGETS),  false },
				{ ARG_STR(TIOCSLCKTRMIOS), true,  true },
				{ ARG_STR(TIOCGLCKTRMIOS), false, true },
			},
			{
				{ (uintptr_t) ARG_STR(NULL), false },
				{ (uintptr_t) (tios1 + 1), NULL, false },
				{ (uintptr_t) tios1 + 4, NULL, false },
				{ (uintptr_t) tios1, NULL, true },
			},
			print_termios, setup_termios, 3
		},
		{
			{
				{ ARG_STR(TCSETA),  true },
				{ ARG_STR(TCSETAW), true },
				{ ARG_STR(TCSETAF), true },
				{ ARG_STR(TCGETA),  false },
			},
			{
				{ (uintptr_t) ARG_STR(NULL), false },
				{ (uintptr_t) (tio + 1), NULL, false },
				{ (uintptr_t) tio + 4, NULL, false },
				{ (uintptr_t) tio, NULL, true },
			},
			print_termio, setup_termio, 3
		},
	};

	ret = open("/dev/ptmx", O_RDWR|O_NOCTTY);
	if (ret < 0)
		perror_msg_and_skip("open(\"/dev/ptmx\")");

	for (size_t i = 0; i < ARRAY_SIZE(checks); i++) {
		const char *last_arg_str = NULL;

		for (size_t j = 0; j < ARRAY_SIZE(checks[0].cmds); j++) {
			size_t k = 0, l = 0;
			bool end = false;
			bool write = checks[i].cmds[j].write;

			if (!checks[i].cmds[j].cmd_str)
				continue;

			while (true) {
				if (write && checks[i].args[k].valid)
					last_arg_str = checks[i].setup(
						(void *) (uintptr_t) (checks[i].args[k].data),
						l);

				do_ioctl(checks[i].cmds[j].cmd,
					 checks[i].cmds[j].cmd_str,
					 ret,
					 checks[i].printer,
					 checks[i].args[k].data,
					 checks[i].args[k].valid,
					 write, last_arg_str,
					 checks[i].cmds[j].can_fail);

				if (k < (ARRAY_SIZE(checks[0].args) - 1))
					k++;
				else if (write && (l < checks[i].setup_variants))
					l++;
				else if (!write && (l < 1))
					l++;
				else
					end = true;

				if (end)
					break;
			}
		}
	}

	puts("+++ exited with 0 +++");

	return 0;
}