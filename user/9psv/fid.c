#include "user.h"
#include "types.h"
#include "param.h"
#include "styx2000.h"
#include "net/byteorder.h"
#include "fcall.h"

static void styx2000_freefid(struct styx2000_fid*);

static void incfidref(void *v) {
  struct styx2000_fid *f;
  f = v;
  if (f) {
    // incref
  }
}

struct styx2000_fidpool* styx2000_allocfidpool() {
  struct styx2000_fidpool *fpool;
  fpool = malloc(sizeof *fpool);
  if (fpool == 0) {
    return 0;
  }
  fpool->destroy = styx2000_freefid;
  if ((fpool->map = allocmap(incfidref)) == 0) {
    free(fpool);
    return 0;
  }
  return fpool;
}

void styx2000_freefidpool(struct styx2000_fidpool *fpool) {
  freemap(fpool->map, (void (*)(void *))fpool->destroy);
  free(fpool);
}

struct styx2000_fid* styx2000_lookupfid(struct styx2000_fidpool *fpool, uint64 fid) {
  return lookupkey(fpool->map, fid);
}

struct styx2000_fid* styx2000_removefid(struct styx2000_fidpool *fpool, uint64 fid) {
  return deletekey(fpool->map, fid);
}

struct styx2000_fid* styx2000_allocfid(
  struct styx2000_fidpool* fpool,
  char* path,
  uint64 fid
) {
  struct styx2000_fid *f;
  f = malloc(sizeof *f);
  if (f == 0) {
    return 0;
  }
  f->fid = fid;
  f->fd = -1;
  f->fpool = fpool;
  f->path = malloc(strlen(path)+1);
  strcpy(f->path, path);
  f->path[strlen(path)] = 0;
  if (caninsertkey(fpool->map, fid, f) == 0) {
    styx2000_freefid(f);
    return 0;
  }
  return f;
}

static void styx2000_freefid(struct styx2000_fid* fid) {
  free(fid->path);
  free(fid);
}

void styx2000_get_dir(struct styx2000_fid* fid) {
  struct dirent de;
  char buf[512], *p;
  strcpy(buf, fid->path);
  p = buf + strlen(buf);
  *p++ = '/';
  while(read(fid->fd, &de, sizeof(de)) == sizeof(de)){
    if(de.inum == 0)
      continue;
    memmove(p, de.name, DIRSIZ);
    p[DIRSIZ] = 0;
    if(stat(buf, &st) < 0){
      printf("ls: cannot stat %s\n", buf);
      continue;
    }
    printf("%s %d %d %d\n", fmtname(buf), st.type, st.ino, st.size);
  }
}
