/*
 * kalloc: allocate KANJI stream structure
 */

#include "kanji.h"

/*
 * ks_termcode_bind: point the Cstrings byte<->Char conversion helpers at the
 * live terminal code flag.  Cfromc()/cfromC() use this so that OS byte strings
 * (argv, getenv, getcwd) are decoded/encoded in the same code the keyboard and
 * display use.  Tools that have no TERM struct (kpp) leave it unset and get
 * the DEF_T_CODE default.
 */
static KS_FLAG *termcode_ptr = NULL;
static KS_FLAG termcode_default;
static int termcode_ready = 0;

static void
termcode_init(void)
{
	if (termcode_ready) return;
	termcode_ready = 1;
	KS_VALUE(termcode_default) = 0;
	KS_INTERP(termcode_default) = KS_KANJI;
#if defined(DEF_T_CODE)
	KS_CODE(termcode_default) = DEF_T_CODE;
#else
	KS_CODE(termcode_default) = KS_JIS;
#endif
	KS_THRU(termcode_default) = 1;
}

void
ks_termcode_bind(KS_FLAG *p)
{
	termcode_ptr = p;
}

unsigned
ks_termcode_flag(void)
{
	if (termcode_ptr != NULL) return KS_VALUE(*termcode_ptr);
	termcode_init();
	return KS_VALUE(termcode_default);
}

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
