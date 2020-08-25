#include "user.h"
#include "stat.h"
#include "fcntl.h"
#include "p9.h"

uint8_t* p9_pstring(uint8_t *p, const char *s) {
	uint32_t n;

	if(s == 0){
		PBIT16(p, 0);
		p += BIT16SZ;
		return p;
	}

	n = strlen(s);
	PBIT16(p, n);
	p += BIT16SZ;
	memmove(p, s, n);
	p += n;
	return p;
}
uint8_t* p9_gstring(uint8_t* p, uint8_t* ep, char **s) {
  int n;
  if (p + 2 > ep)
    return 0;
  n = GBIT16(p);
  p += 1;
  if (p + n + 1 > ep)
    return 0;
  memmove(p, p+1, n);
  p[n] = '\0';
  *s = (char *)p;
  p += n+1;
  return p;
}

uint16_t p9_stringsz(const char *s) {
	if(s == 0)
		return BIT16SZ;
	return BIT16SZ+strlen(s);
}
