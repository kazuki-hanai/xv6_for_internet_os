#include "user.h"
#include "styx2000.h"
#include "net/byteorder.h"
#include "fcall.h"

#define STYX2000_NOFILE       0
#define STYX2000_NODIRENT     1
#define STYX2000_FILEEXISTS   2
#define STYX2000_NOTFOUND     3
#define STYX2000_NOTDIR       4


static const char* errmap[] = {
	[STYX2000_NOFILE]       "No such file or directory", 
	[STYX2000_NODIRENT]     "directory entry not found",
	[STYX2000_FILEEXISTS]   "File exists",
	[STYX2000_NOTFOUND]     "file not found",
	[STYX2000_NOTDIR]       "Not a directory",
};

const char* styx2000_geterrstr(int key) {
  return errmap[key];
}

int styx2000_compose_rerror(struct styx2000_fcall *f, uint8_t* buf) {
  buf = styx2000_pstring(buf, f->ename);
  return 0;
}
