#ifndef _CX_DEF_
extern unsigned int Cxstr[];
#define _CX_DEF_
#endif /*_CX_DEF_*/
/*	PATH:	This file contains certain info needed to locate the
		MicroEMACS files on a system dependant basis.

									*/

/*	possible names and paths of help files */

Char *pathname[] =
{
	CSTR(&Cxstr[3141]),
	CSTR(&Cxstr[3151]),
	CSTR(&Cxstr[3161]),		/* Don't forget last '/' */
	CSTR(&Cxstr[6])
};
/* In this file, `/usr/local' will be replaced to $(PREFIX) in mf.c
   before compiling */

#define NPNAMES (sizeof(pathname)/sizeof(Char *))
