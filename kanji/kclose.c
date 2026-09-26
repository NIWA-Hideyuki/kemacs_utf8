#include "kanji.h"

void
kclose(KFILE * kp)
{

	(void)kfree((KSTREAM *)kp);
}
