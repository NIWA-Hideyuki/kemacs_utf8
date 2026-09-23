/*
 * converts Shift-JIS 2-byte code to JIS 2-byte code
 * returns 0 on successful completion
 *         1 if error
 */

int
stoj(int * s, int * j)
{

	register int sh = ((*s >> 8) & 0377), sl = (*s & 0377);
	register int jh, jl;

	if (!(0x81 <= sh && (sh <= 0x9f || (0xe0 <= sh && sh <= 0xfc))) ||
	    !(0x40 <= sl && sl <= 0xfc && sl != 0x7f)) return 1;
	jh = ((sh - ((sh >= 0xa0)? 0xc1: 0x81)) << 1)+0x21;
	if (sl >= 0x9f) {
		jh++;
		jl = sl - 0x7e;
	} else {
		jl = sl - ((sl <= 0x7e) ? 0x1f : 0x20);
	}
	if (jh > 0x7e) /* in this case, 0x8000 bit of return value is set */
		jh += (jh<=0x83 && jh!=0x80 ? 0x22 : jh<=0x87 ? 0x28 : 0x66);
	*j = (jh << 8) | jl;
	return 0;
}