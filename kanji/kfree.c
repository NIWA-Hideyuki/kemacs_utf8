/*
 * kfree: free KSTREAM structure
 */

#include "kanji.h"

#define JKANJI	1
#define KANA	2

int
kfree(KSTREAM * ksp)
{

	if (ksp->ks_putf && KS_CODE(ksp->ks_flag) == KS_JIS) {
		static char b[3];

		if (ksp->ks_pstate & KANA) {
			b[0] = SHFTIN;
			(void)(*ksp->ks_putf)(ksp->ks_id, b, 1);
		}
		if (ksp->ks_pstate & JKANJI) {
			b[0] = ESCAPE;
			b[1] = '(';
			b[2] = (KS_RI(ksp->ks_flag) == KS_ROMAJI)? 'J': 'B';
			(void)(*ksp->ks_putf)(ksp->ks_id, b, 3);
		}
	}
	(void)(*ksp->ks_closef)(ksp->ks_id);
	free((char *)ksp);
	return 0;
}