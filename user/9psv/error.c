#include "user.h"
#include "p9.h"
#include "net/byteorder.h"

#define P9_NOFILE       0
#define P9_NODIRENT     1
#define P9_FILEEXISTS   2
#define P9_NOTFOUND     3
#define P9_NOTDIR       4


static const char* errmap[] = {
	[P9_NOFILE]       "No such file or directory", 
	[P9_NODIRENT]     "directory entry not found",
	[P9_FILEEXISTS]   "File exists",
	[P9_NOTFOUND]     "file not found",
	[P9_NOTDIR]       "Not a directory",
};

const char* p9_geterrstr(int key) {
  return errmap[key];
}

int p9_compose_rerror(struct p9_fcall *f, uint8_t* buf) {
  buf = p9_pstring(buf, f->ename);
  return 0;
}
