/*
 * kalloc: allocate KANJI stream structure
 */

#include "kanji.h"

unsigned ks_binmode_flag(void)
{
	static KS_FLAG flg;
	static int first = 1;
	if(first){
		first = 0;
		KS_THRU(flg) = 1;
		KS_INTERP(flg) = KS_BINARY;
	}
	return flg.u_flag;
}

KSTREAM *
kalloc(caddr_t id, int (*openf)(caddr_t), int (*closef)(caddr_t), int (*getf)(caddr_t, char *, int), int (*putf)(caddr_t, char *, int), unsigned flag)
{

	KSTREAM *ksp;

	if ((!getf && !putf) ||
	     !(ksp = (KSTREAM *)malloc(sizeof(KSTREAM)))) return NULL;
	ksp->ks_id = id;
	ksp->ks_openf = openf;
	ksp->ks_closef = closef;
	ksp->ks_getf = getf;
	ksp->ks_putf = putf;
	KS_VALUE(ksp->ks_flag) = flag;
	ksp->ks_gstate = ksp->ks_pstate = 0;
	ksp->ks_ql = ksp->ks_ul = ksp->ks_bl = 0;
	(void)(*ksp->ks_openf)(id);
	return ksp;
}