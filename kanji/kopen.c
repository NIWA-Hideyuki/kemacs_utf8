#include "kanji.h"
#include <unistd.h>
#if HAVE_SELECT
# include <sys/ioctl.h>
# include <sys/time.h>
#else
# include <signal.h>
#endif
#include <errno.h>

/*
 * open Kanji stream on opened file stream fp.
 * kpass specifies if passes shift-in/out through.
 * programs which needs to catch ^N/^O must set it.
 */

struct KF {
	FILE *fp;			/* associated FILE pointer */
#if HAVE_SELECT
	struct timeval tm;	/* ESC timeout (passed to select) */
#else
	unsigned tm;		/* passed to alarm */
	SIGRET_T (*ac)(void);		/* last alarm handler */
	unsigned lt;		/* last alarm count rest */
#endif
	int lastesc;		/* last input was ESC */
};

static int f_open(struct KF *), f_close(struct KF *), f_get(struct KF *, char *, int), f_put(struct KF *, char *, int);

#if !HAVE_SELECT
/* ARGSUSED */
static SIGRET_T
void
{

}
#endif

KFILE *
kopen(FILE * fp, unsigned flag, unsigned totime)
{


	register KSTREAM *kp;
	register struct KF *kfp;

	if (!fp
	    || !(kfp = (struct KF *)malloc(sizeof(struct KF)))) return NULL;
	kfp->fp = fp;
#if HAVE_SELECT
	kfp->tm.tv_sec = totime/1000;
	kfp->tm.tv_usec = (totime % 1000) * 1000;
#else
	kfp->tm = totime? 1: 0;
#endif
	if (!(kp = kalloc((caddr_t)kfp, (int (*)(caddr_t))f_open, (int (*)(caddr_t))f_close, (int (*)(caddr_t, char *, int))f_get, (int (*)(caddr_t, char *, int))f_put, flag)))
		return NULL;
	return kp;
}

/*ARGSUSED*/
static int
f_open(struct KF * id)
{

	id->lastesc = 0;
	return 0;
}

static int
f_close(struct KF * id)
{

	return fflush(id->fp);
}

static int
f_get(struct KF * id, char * buf, int len)
{

	register int n;
#if 0
	extern int errno;
#endif

	if (id->lastesc) {
		/* check timeout */
#if HAVE_SELECT
		if (id->tm.tv_usec || id->tm.tv_sec) {
			fd_set r;
			struct timeval tm;

		    retry:
		    	FD_ZERO(&r);
		    	FD_SET(fileno(id->fp), &r);
			tm.tv_usec = id->tm.tv_usec;
			tm.tv_sec = id->tm.tv_sec;
			 /* explicitly set timeout content
			    each time select() is called */
			n = select(fileno(id->fp)+1, &r, NULL, NULL, &tm);
			if (!n) return 0;
			if (n < 0) {
				if (errno == EINTR) goto retry;
				return -1;
			}
		}
#else /* !HAVE_SELECT */
		if (id->tm) {
			id->ac = signal(SIGALRM, alarm_catcher);
			id->lt = alarm(id->tm);
		}
#endif
	}
	n = read(fileno(id->fp), buf, len);
	if (n < 0 && errno == EINTR) {
		/* timed out */
#if !HAVE_SELECT
		(void)signal(SIGALRM, id->ac);
		(void)alarm(id->lt);
#endif
		return 0;
	}
	id->lastesc = (n > 0 && buf[n-1] == ESCAPE);
	return (n<=0)? -1: n;
}

static int
f_put(struct KF * id, char * buf, int len)
{

	(void)fwrite(buf, sizeof(char), len, id->fp);
	return len;
}