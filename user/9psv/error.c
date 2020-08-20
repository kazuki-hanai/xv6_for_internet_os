#include "user.h"
#include "p9.h"
#include "net/byteorder.h"

static const char* errmap[] = {
	[P9_NOFILE]       "No such file or directory", 
	[P9_NODIRENT]     "directory entry not found",
	[P9_FILEEXISTS]   "File exists",
	[P9_NOTFOUND]     "file not found",
	[P9_NOTDIR]       "Not a directory",
  [P9_UNKNOWNFID]   "Unknown fid",
  [P9_PERM]         "Permission denied"
};

const char* p9_geterrstr(int key) {
  return errmap[key];
}

int p9_compose_rerror(struct p9_fcall *f, uint8_t* buf) {
  buf = p9_pstring(buf, f->ename);
  return 0;
}
