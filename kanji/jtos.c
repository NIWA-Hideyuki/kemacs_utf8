/*
 * converts JIS 2-byte code to Shift-JIS 2-byte code
 * returns 0 on successful conversion
 *         1 if error
 */

int
jtos(int * j, int * s)
{

	int jh = ((*j >> 8) & 0377), jl = (*j & 0377);
	int sh, sl;

	if (jh <= ' ' || jh >= '\177' || jl <= ' ' || jl >= '\177') return 1;
	sh = ((jh - 0x21) >> 1) + 0x81;
	if (sh > 0x9f)
		sh += 0x40;
	if (jh & 1) {
		sl = jl + 0x1f;
		if (jl > 0x5f)
			sl++;
	} else
		sl = jl + 0x7e;
	*s = (sh << 8) | sl;
	return 0;
}