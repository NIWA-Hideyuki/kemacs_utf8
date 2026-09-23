#ifndef _CX_DEF_
extern short Cxstr[];
#define _CX_DEF_
#endif /*_CX_DEF_*/
/*	EVAL.C:	Expresion evaluation functions for
		MicroEMACS

	written 1986 by Daniel Lawrence				*/

/* Atsuo Ohki -- 14 Dec, 1988
   some macro functions do not work properly,
   because Cstrncpy() may not terminate destination string with null */


#include	"ecomm.h"
#include	"estruct.h"
#include	"edef.h"
#include	"evar.h"

Char *itoaa(int i);

varinit(void)		/* initialize the user variable list */
{
	register int i;

	for (i=0; i < MAXVARS; i++)
		uv[i].u_name[0] = 0;
}

static Char *
gtfun(fname)	/* evaluate a function */
	Char *fname;		/* name of function to evaluate */
{
	register int fnum;		/* index to function to eval */
	Char arg1[NSTRING];		/* value of first argument */
	Char arg2[NSTRING];		/* value of second argument */
	Char arg3[NSTRING];		/* value of third argument */
	static Char result[2 * NSTRING];	/* string result */
	int len;

	/* look the function up in the function table */
	fname[3] = 0;	/* only first 3 chars significant */
	(void)mklower(fname);	/* and let it be upper or lower case */
	for (fnum = 0; fnum < NFUNCS; fnum++)
		if (Cstrcmp(fname, funcs[fnum].f_name) == 0)
			break;

	/* return errorm on a bad reference */
	if (fnum == NFUNCS)
		return(errorm);

	/* if needed, retrieve the first argument */
	if (funcs[fnum].f_type != NILNAMIC) {
	  if (macarg(arg1) != TRUE)
	    return(errorm);

	  /* if needed, retrieve the second argument */
	  if (funcs[fnum].f_type != MONAMIC) {
	    if (macarg(arg2) != TRUE)
	      return(errorm);

	    /* if needed, retrieve the third argument */
	    if (funcs[fnum].f_type != DYNAMIC)
	      if (macarg(arg3) != TRUE)
		return(errorm);
	  }
	}

	/* and now evaluate it! */
	switch (fnum) {
	case UFADD:	return(itoaa(stoi(arg1) + stoi(arg2)));
	case UFSUB:	return(itoaa(stoi(arg1) - stoi(arg2)));
	case UFTIMES:	return(itoaa(stoi(arg1) * stoi(arg2)));
	case UFDIV:	return(itoaa(stoi(arg1) / stoi(arg2)));
	case UFMOD:	return(itoaa(stoi(arg1) % stoi(arg2)));
	case UFNEG:	return(itoaa(-stoi(arg1)));
	case UFCAT:	(void)Cstrcpy(result, arg1);
			return(Cstrcat(result, arg2));
	case UFLEFT:	(void)Cstrncpy(result, arg1, len=stoi(arg2));
			result[len] = 0;	/* make sure null terminated */
			return(result);
	case UFRIGHT:	return(Cstrcpy(result, &arg1[stoi(arg2)-1]));
	case UFMID:	(void)Cstrncpy(result, &arg1[stoi(arg2)-1],
				len=stoi(arg3));
			result[len] = 0;	/* make sure null terminated */
			return(result);
	case UFNOT:	return(ltos(stol(arg1) == FALSE));
	case UFEQUAL:	return(ltos(stoi(arg1) == stoi(arg2)));
	case UFLESS:	return(ltos(stoi(arg1) < stoi(arg2)));
	case UFGREATER:	return(ltos(stoi(arg1) > stoi(arg2)));
	case UFSEQUAL:	return(ltos(Cstrcmp(arg1, arg2) == 0));
	case UFSLESS:	return(ltos(Cstrcmp(arg1, arg2) < 0));
	case UFSGREAT:	return(ltos(Cstrcmp(arg1, arg2) > 0));
	case UFIND:	return(getval(arg1));
	case UFENV:	{	char *s;
			return((s = getenv(cfromC(arg1)))? SafeCfromc(s):
							   CSTR(&Cxstr[6]));
		}
	case UFLENGTH:	return(itoaa(Cstrlen(arg1)));
	case UFAND:	return(ltos(stol(arg1) && stol(arg2)));
	case UFOR:	return(ltos(stol(arg1) || stol(arg2)));
	case UFUPPER:	return(mkupper(arg1));
	case UFLOWER:	return(mklower(arg1));
     /*	case UFTRUTH:	return(ltos(stoi(arg1) == 42)); */ /* ?? Nide */
	case UFTRUTH:	return(ltos(stoi(arg1) != 0));
	case UFASCII:	return(itoaa((int)arg1[0]));
	case UFCHR:	result[0] = stoi(arg1);
			result[1] = 0;
			return(result);
	case UFGTKEY:	result[0] = tgetc();
			result[1] = 0;
			return(result);
	case UFRND:	return(itoaa((ernd() % abs(stoi(arg1))) + 1));
	case UFABS:	return(itoaa(abs(stoi(arg1))));
	case UFBIND:
	    {
		register Char *p;

		if ((p = key2fname(strcmd(arg1))) == NULL || *p == '\0')
		    return errorm;
		return Cstrcpy(result, p);
	    }

	case UFBMODE:
	case UFGMODE:
	    {
		int i;
		
		(void)mkupper(arg1);
		for(i = 0; modename[i] != NULL; i++)
			if(Cstrcmp(modename[i], arg1) == 0)
				return ltos((i << i) & (fnum == UFGMODE ?
					gmode : curwp->w_bufp->b_mode));
		return errorm;
	    }
	}

	exit(-11);	/* never should get here */
	/*NOTREACHED*/
}

static Char *
gtusr(vname)	/* look up a user var's value */
	Char *vname;		/* name of user variable to fetch */
{

	register int vnum;	/* ordinal number of user var */

	/* scan the list looking for the user var name */
	for (vnum = 0; vnum < MAXVARS; vnum++)
		if (Cstrcmp(vname, uv[vnum].u_name) == 0)
			break;

	/* return errorm on a bad reference */
	if (vnum == MAXVARS)
		return(errorm);

	return(uv[vnum].u_value);
}

static Char *
gtenv(vname)
	Char *vname;		/* name of environment variable to retrieve */
{
	register ENVAR *evp;	/* ordinal number of var refrenced */

	/* scan the list, looking for the referenced name */
	for (evp = envars; evp < envars+NEVARS; evp++) {
		if (Cstrcmp(vname, evp->ev_name) == 0)
			return *evp->ev_gethook ? (*evp->ev_gethook)(evp) :
				(Char *)evp->ev_addr;
	}

	/* return errorm on a bad reference */
	return(errorm);
}

/*ARGSUSED*/
static int
readonly_hook(int evp, int val)
{
	mlwrite(
#if KMSGS
		kterminal? CSTR(&Cxstr[4178]):
#endif
		CSTR(&Cxstr[4193]));
	return(FALSE);
}

static Char *
get_int_hook(ENVAR * evp)
{

	if (evp->ev_class != EV_LOCAL)
		return itoaa(*((int *)evp->ev_addr));
	return itoaa(*((int *)BUF_ADR(evp->ev_addr)));
}

static Char *
get_bool_hook(evp)
	register ENVAR *evp;
{
	if (evp->ev_class != EV_LOCAL)
		return ltos(*((bool *)evp->ev_addr));
	return ltos(*((bool *)BUF_ADR(evp->ev_addr)));
}

static Char *
get_str_hook(evp)
	register ENVAR *evp;
{
	register Char *p;

	p = (evp->ev_class != EV_LOCAL)?
		*((Char **)evp->ev_addr):
		*((Char **)BUF_ADR(evp->ev_addr));
	return (!p? CSTR(&Cxstr[6]): p);
}

static int
put_int_hook(evp, val)
	register ENVAR *evp;
	Char *val;
{
	if (evp->ev_class != EV_LOCAL)
		*((int *)evp->ev_addr) = stoi(val);
	else
		*((int *)BUF_ADR(evp->ev_addr)) = stoi(val);
	return(TRUE);
}

static int
put_bool_hook(evp, val)
	register ENVAR *evp;
	Char *val;
{
	if (evp->ev_class != EV_LOCAL)
		*((bool *)evp->ev_addr) = stol(val);
	else
		*((bool *)BUF_ADR(evp->ev_addr)) = stol(val);
	return(TRUE);
}

static int
put_str_hook(evp, val)
	register ENVAR *evp;
	Char *val;
{
	register Char **p;

	if (evp->ev_class != EV_LOCAL)
		p = (Char **)evp->ev_addr;
	else
		p = (Char **)BUF_ADR(evp->ev_addr);
	return copystr(p, val);
}

/*ARGSUSED*/
static Char *
get_pagelen_hook(evp)
	ENVAR *evp;
{
	return itoaa(term.t_nrow+1);
}

/*ARGSUSED*/
static int
put_pagelen_hook(evp, val)
	ENVAR *evp;
	Char *val;
{
	return newsize(TRUE, stoi(val));
}

/*ARGSUSED*/
static Char *
get_curcol_hook(evp)
	ENVAR *evp;
{
	return(itoaa(getccol(FALSE)+1));
}

/*ARGSUSED*/
static int
put_curcol_hook(evp, val)
	ENVAR *evp;
	Char *val;
{
	return setccol(stoi(val));
}

/*ARGSUSED*/
static Char *
get_curline_hook(evp)
	ENVAR *evp;
{
	return(itoaa(getcline()));
}

/*ARGSUSED*/
static int
put_curline_hook(evp, val)
	ENVAR *evp;
	Char *val;
{
	return gotoline(TRUE, stoi(val));
}

/*ARGSUSED*/
static int
put_curwidth_hook(evp, val)
	ENVAR *evp;
	Char *val;
{
	return newwidth(TRUE, stoi(val));
}

/*ARGSUSED*/
static int
put_cbufname_hook(evp, val)
	ENVAR *evp;
	Char*val;
{
	if (put_str_hook(evp, val) != TRUE)
		return(FALSE);
	curwp->w_flag |= WFMODE;
	return(TRUE);
}

/*ARGSUSED*/
static int
put_cfname_hook(evp, val)
	ENVAR *evp;
	Char *val;
{
	if (put_str_hook(evp, val) != TRUE)
		return(FALSE);
	curwp->w_flag |= WFMODE;
	return(TRUE);
}

#if	KANJI
static int
put_kterminal_hook(evp, val)
	register ENVAR *evp;
	Char *val;
{
	if (put_bool_hook(evp, val) != TRUE)
		return(FALSE);
	if (blistp)
		(void)copystr(&blistp->b_bname, buffer_list_buffer_name());
	if (bdlistp)
		(void)copystr(&bdlistp->b_bname, binding_list_buffer_name());
	if (hlpbp)
		(void)copystr(&hlpbp->b_bname, help_buffer_name());
	upmode();
	return(TRUE);
}

static Char *
get_kcode_hook(evp)
     register ENVAR *evp;
{
	if (evp->ev_class != EV_LOCAL)
		return code_string(*(KS_FLAG *)evp->ev_addr);
	return code_string(*(KS_FLAG *)BUF_ADR(evp->ev_addr));
}

static int
put_kcode_hook(evp, val)
     register ENVAR *evp;
     register Char *val;
{
	if (evp->ev_class != EV_LOCAL)
		string_code(val, (KS_FLAG *)evp->ev_addr);
	else
		string_code(val, (KS_FLAG *)BUF_ADR(evp->ev_addr));
	return (TRUE);
}

static int
put_tcode_hook(evp, val)
     register ENVAR *evp;
     register Char *val;
{
  if (put_kcode_hook(evp, val) != TRUE)
  	return(FALSE);
  TTkclose();
  TTkopen();
  return (TRUE);
}

/*ARGSUSED*/
static int
put_bufcode_hook(evp, val)
     register ENVAR *evp;
     register Char *val;
{
  if (put_kcode_hook(evp, val) != TRUE)
	return(FALSE);
  curwp->w_flag |= WFMODE;
  return (TRUE);
}
#endif /* KANJI */

/*ARGSUSED*/
static Char *
get_curchar_hook(evp)
	ENVAR *evp;
{
	return(itoaa(lgetc(curwp->w_dotp, curwp->w_doto)));
}

/*ARGSUSED*/
static int
put_curchar_hook(evp, val)
	ENVAR *evp;
	register Char *val;
{
	register Char c;

	(void)ldelete(1L, FALSE);	/* delete 1 char */
	c = stoi(val);
	if (c == '\n')
		(void)lnewline();
	else
		(void)linsert(1, c);
}

static int
put_modeline_hook(evp, val)
	register ENVAR *evp;
	register Char *val;
{
	if (put_str_hook(evp, val) != TRUE)
		return(FALSE);
	curwp->w_flag |= WFMODE;
	return(TRUE);
}

#if	CLOCK
static int
put_cperiod_hook(evp, val)
	register ENVAR *evp;
	register Char *val;
{
	if (put_int_hook(evp, val) != TRUE)
		return(FALSE);
	clinit();
	return(TRUE);
}
#endif /* CLOCK */

int
setvar(f, n)		/* set a variable */
{
	register int vnum;	/* ordinal number of var refrenced */
	register int status;	/* status return */
	register int vtype;	/* type of variable to set */
	Char var[NVSIZE+1];	/* name of variable to fetch */
	Char val[NSTRING];	/* value to set variable to */
	register ENVAR *evp;

	/* first get the variable to set.. */
	if (clexec == FALSE) {
		status = mlreply(
#if KMSGS
				 kterminal? CSTR(&Cxstr[4220]):
#endif
				 CSTR(&Cxstr[4226]),
				&var[0], NVSIZE);
		if (status != TRUE)
			return(status);
	} else {	/* macro line argument */
		/* grab token and skip it */
		execstr = token(execstr, var);
	}

	/* check the legality and find the var */
    sv01:
	vtype = -1;
	switch (var[0]) {
	case '$': /* check for legal enviromnent var */
		for (evp = envars; evp < envars+NEVARS; evp++)
			if (Cstrcmp(&var[1], evp->ev_name) == 0) {
				vtype = TKENV;
				break;
			}
		break;
	case '%': /* check for existing legal user variable */
		for (vnum = 0; vnum < MAXVARS; vnum++)
			if (Cstrcmp(&var[1], uv[vnum].u_name) == 0) {
				vtype = TKVAR;
				break;
			}
		if (vnum < MAXVARS)
			break;

		/* create a new one??? */
		for (vnum = 0; vnum < MAXVARS; vnum++)
			if (uv[vnum].u_name[0] == 0) {
				vtype = TKVAR;
				(void)Cstrcpy(uv[vnum].u_name, &var[1]);
				break;
			}
		break;
	case '&':	/* indirect operator? */
		var[4] = 0;
		if (Cstrcmp(&var[1], CSTR(&Cxstr[3283])) == 0) {
			/* grab token, and eval it */
			execstr = token(execstr, var);
			(void)Cstrcpy(var, getval(var));
			goto sv01;
		}
		break;
	}

	/* if its not legal....bitch */
	if (vtype == -1) {
		mlwrite(CSTR(&Cxstr[4244]),
#if KMSGS
			kterminal? CSTR(&Cxstr[4247]):
#endif
			CSTR(&Cxstr[4262]));
		return(FALSE);
	}

	/* get the value for that variable */
	if (f == TRUE)
		(void)Cstrcpy(val, itoaa(n));
	else {
		status = mlreply(
#if KMSGS
				 kterminal? CSTR(&Cxstr[4280]):
#endif
				 CSTR(&Cxstr[4284]),
				val, NSTRING);
		if (status != TRUE)
			return(status);
	}

	/* and set the appropriate value */
	status = TRUE;
	switch (vtype) {
	case TKVAR: /* set a user variable */
		status = copystr(&uv[vnum].u_value, val);
		break;

	case TKENV: /* set an environment variable */
		status = *evp->ev_puthook ? (*evp->ev_puthook)(evp, val) :
			NULL != Cstrcpy(evp->ev_addr, val);
		break;
	}
	thisflag = lastflag;	/* "set" should not affect goal column. */
	return(status);
}

/*	stoi:	ascii string to integer......This is too
		inconsistant to use the system's	*/
int
stoi(st)
	Char *st;
{
	int result;	/* resulting number */
	int sign;	/* sign of resulting number */
	Char c;		/* current char being examined */

	result = 0;
	sign = 1;

	/* skip preceding whitespace */
	while (*st == ' ' || *st == '\t')
		++st;

	/* check for sign */
	if (*st == '-') {
		sign = -1;
		++st;
	}
	if (*st == '+')
		++st;

	/* scan digits, build value */
	while ((c = *st++)) {
	  if (isdigit(c))
	    result = result * 10 + c - '0';
	  else
	    return(0);
	}
	return(result * sign);
}

/*	itoaa:	integer to ascii string.......... This is too
		inconsistant to use the system's	*/
Char *
itoaa(i)
	int i;	/* integer to translate to a string */
{
	register int digit;		/* current digit being used */
	register Char *sp;		/* pointer into result */
	register int sign;		/* sign of resulting number */
	static Char result[INTWIDTH+1];	/* resulting string */

	/* eliminate the trivial 0 */
	if (i == 0)
		return(CSTR(&Cxstr[1293]));

	/* record the sign...*/
	sign = 1;
	if (i < 0) {
		sign = -1;
		i = -i;
	}

	/* and build the string (backwards!) */
	sp = result + INTWIDTH;
	*sp = '\0';
	while (i) {
		digit = i % 10;
		*(--sp) = '0' + digit;	/* and install the new digit */
		i = i / 10;
	}

	/* and fix the sign */
	if (sign == -1) {
		*(--sp) = '-';	/* and install the minus sign */
	}

	return(sp);
}

int
gettyp(tok)	/* find the type of a passed token */
	Char *tok;	/* token to analyze */
{
	register Char c;	/* first char in token */

	/* grab the first char (this is all we need) */
	c = *tok;

	/* no blanks!!! */
	if (c == 0)
		return(TKNUL);

	/* a numeric literal? */
	if (isdigit(c))
		return(TKLIT);

	switch (c) {
	case '"':	return(TKSTR);

	case '!':	return(TKDIR);
	case '@':	return(TKARG);
	case '#':	return(TKBUF);
	case '$':	return(TKENV);
	case '%':	return(TKVAR);
	case '&':	return(TKFUN);
	case '*':	return(TKLBL);

	default:	return(TKCMD);
	}
}

Char *
getval(tok)	/* find the value of a token */
	Char *tok;		/* token to evaluate */
{
	register int status;	/* error return */
	register BUFFER *bp;	/* temp buffer pointer */
	register int blen;	/* length of buffer argument */
	static Char buf[NSTRING]; /* string buffer for some returns */

	switch (gettyp(tok)) {
	case TKNUL:	return(CSTR(&Cxstr[6]));

	case TKARG:	/* interactive argument */
			(void)Cstrcpy(tok, getval(&tok[1]));
			status = getstring(tok,
				   buf, NSTRING, ctoec('\n'));
			if (status == ABORT)
				return(errorm);
			return(buf);

	case TKBUF:	/* buffer contents fetch */

			/* grab the right buffer */
			(void)Cstrcpy(tok, getval(&tok[1]));
			if (!(bp = bfind(tok, FALSE, 0)))
				return(errorm);

			/*
			 * if the buffer is displayed, get the window
			 * vars instead of the buffer vars
			 */
			if (bp->b_nwnd > 0) {
			  curbp->b_dotp = curwp->w_dotp;
			  curbp->b_doto = curwp->w_doto;
			}

			/* make sure we are not at the end */
			if (bp->b_linep == bp->b_dotp)
				return(errorm);

			/* grab the line as an argument */
			blen = bp->b_dotp->l_used - bp->b_doto;
			if (blen > NSTRING)
				blen = NSTRING;
			(void)Cstrncpy(buf, bp->b_dotp->l_text + bp->b_doto,
				       blen);
			buf[blen] = 0;
	
			/* and step the buffer's line ptr ahead a line */
			bp->b_dotp = bp->b_dotp->l_fp;
			bp->b_doto = 0;

			/* if displayed buffer, reset window ptr vars*/
			if (bp->b_nwnd > 0) {
				curwp->w_dotp = curbp->b_dotp;
				curwp->w_doto = 0;
				curwp->w_flag |= WFMOVE;
			}

			/* and return the spoils */
			return(buf);		

	case TKVAR:	return(gtusr(tok+1));
	case TKENV:	return(gtenv(tok+1));
	case TKFUN:	return(gtfun(tok+1));
	case TKDIR:	return(errorm);
	case TKLBL:	return(itoaa(gtlbl(tok)));
	case TKLIT:	return(tok);
	case TKSTR:	return(tok+1);
	case TKCMD:	return(tok);
	}
	/*NOTREACHED*/
}

/*ARGSUSED*/
int
gtlbl(tok)	/* find the line number of the given label */
	Char *tok;	/* label name to find */
{
	return(1);
}

int
stol(val)	/* convert a string to a numeric logical */
	Char *val;	/* value to check for stol */
{
	/* check for logical values */
	if (val[0] == 'F' || val[0] == 'f')
		return(FALSE);
	if (val[0] == 'T' || val[0] == 't')
		return(TRUE);

	/* check for numeric truth (!= 0) */
	return((stoi(val) != 0));
}

Char *
ltos(val)		/* numeric logical to string logical */
	int val;	/* value to translate */
{
	if (val)
		return(truem);
	else
		return(falsem);
}

#if	KANJI
Char *
code_string(code)
     KS_FLAG code;
{
  static Char b[20]; /* max: CRLF/JIS-NEW-ROMAJI */

  switch (KS_EOL(code)) { /* Added by Nide */
  case KS_LF:
    b[0] = 0;
    break;
  case KS_CR:
    (void)Cstrcpy(b, CSTR(&Cxstr[4292]));
    break;
  case KS_CRLF:
    (void)Cstrcpy(b, CSTR(&Cxstr[4296]));
    break;
  default:
    (void)Cstrcpy(b, CSTR(&Cxstr[4302]));
    break;
  }

  switch (KS_CODE(code)) {
  case KS_JIS:
    (void)Cstrcat(b, CSTR(&Cxstr[4307]));
    switch (KS_KI(code)) {
    case KS_NEWJIS:
      (void)Cstrcat(b, CSTR(&Cxstr[4311]));
      break;
    case KS_OLDJIS:
      (void)Cstrcat(b, CSTR(&Cxstr[4316]));
      break;
    default:
      (void)Cstrcat(b, CSTR(&Cxstr[4321]));
      break;
    }
    switch(KS_RI(code)) {
    case KS_ROMAJI:
      (void)Cstrcat(b, CSTR(&Cxstr[4326]));
      break;
    case KS_ASCII:
      (void)Cstrcat(b, CSTR(&Cxstr[4334]));
      break;
    case KS_BOGUS:
      (void)Cstrcat(b, CSTR(&Cxstr[4341]));
      break;
    default:
      (void)Cstrcat(b, CSTR(&Cxstr[4321]));
      break;
    }
    break;
  case KS_UJIS:
    (void)Cstrcat(b, CSTR(&Cxstr[4348]));
    break;
  case KS_SJIS:
    (void)Cstrcat(b, CSTR(&Cxstr[4353]));
    break;
#if HANDLE_UTF
  case KS_UTF8:
    (void)Cstrcat(b, CSTR(&Cxstr[4358]));
    break;
#endif
  default:
    (void)Cstrcat(b, CSTR(&Cxstr[4322]));
    break;
  }
  return b;
}

string_code(str, code)
     Char *str;		/* code specifying string */
     KS_FLAG *code;	/* default value of code */
{
  register Char *p, *q;

  if(p = Cindex(str, '/'), p++ == NULL){ /* Added by Nide */
    p = str;
    KS_EOL(*code) = KS_LF;
  } else switch (*str) {
  case 'L': case 'l': /* "LF/" case */
    KS_EOL(*code) = KS_LF;
    break;
  case 'C': case 'c': /* "CR/" or "CRLF/" case */
    KS_EOL(*code) = KS_CR;
    for(q = str+1; q < p; q++){
      if(*q == 'L' || *q == 'l'){
	KS_EOL(*code) = KS_CRLF;
	break;
      }
    }
    break;
  }

  switch (*p) {
  case 'j': case 'J':
    KS_CODE(*code) = KS_JIS;
    if ((p = Cindex(p, '-')) == NULL) return;
    switch (*++p) {
    case 'n': case 'N':
      KS_KI(*code) = KS_NEWJIS;
      break;
    case 'o': case 'O':
      KS_KI(*code) = KS_OLDJIS;
      break;
    }
    if ((p = Cindex(p, '-')) == NULL) return;
    switch (*++p) {
    case 'r': case 'R':
      KS_RI(*code) = KS_ROMAJI;
      break;
    case 'a': case 'A':
      KS_RI(*code) = KS_ASCII;
      break;
    case 'b': case 'B':
      KS_RI(*code) = KS_BOGUS;
      break;
    }
    return;
  case 'u': case 'U':
    switch (*++p) {
    case 't': case 'T':
#if HANDLE_UTF
      KS_CODE(*code) = KS_UTF8;
      break;
#endif
    default:
      KS_CODE(*code) = KS_UJIS;
      break;
    }
    return;
  case 's': case 'S':
    KS_CODE(*code) = KS_SJIS;
    return;
  }
  return;
}
#endif /* KANJI */

/* make a string upper case */
Char *
mkupper(str)
     Char *str;		/* string to upper case */
{
	register Char *sp;

	sp = str;
	while (*sp) {
		if (islower(*sp)) *sp ^= DIFCASE;
		++sp;
	}
	return(str);
}

/* make a string lower case */
Char *
mklower(str)
     Char *str;		/* string to lower case */
{
  register Char *sp;

  sp = str;
  while (*sp) {
    if (isupper(*sp))
      *sp ^= DIFCASE;
    ++sp;
  }
  return(str);
}

/* take the absolute value of an integer */
int
abs(x)
     int x;
{
	return(x < 0 ? -x : x);
}

/* returns a random integer */
int
ernd()
{
	seed = abs(seed * 1721 + 10007);
	return(seed);
}