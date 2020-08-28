#include "user.h"
#include "stat.h"
#include "fcntl.h"
#include "../p9.h"
#include "net/byteorder.h"

uint8_t* p9_parse_tstat(struct p9_fcall *fcall, uint8_t* buf, int len) {
  fcall->fid = GBIT32(buf);
  buf += 4;
  return buf;
}

int p9_compose_rstat(struct p9_fcall *f, uint8_t* buf) {
  PBIT16(buf, f->parlen);
  buf += BIT16SZ;
  PBIT16(buf, f->nstat);
  buf += BIT16SZ;
  PBIT16(buf, f->stat->type);
  buf += BIT16SZ;
  PBIT32(buf, f->stat->dev);
  buf += BIT32SZ;
  
  PBIT8(buf, f->statqid->type);
  buf += BIT8SZ;
  PBIT32(buf, f->statqid->vers);
  buf += BIT32SZ;
  PBIT64(buf, f->statqid->path);
  buf += BIT64SZ;

  PBIT32(buf, f->stat->mode);
  buf += BIT32SZ;
  PBIT32(buf, f->stat->atime);
  buf += BIT32SZ;
  PBIT32(buf, f->stat->mtime);
  buf += BIT32SZ;
  PBIT64(buf, f->stat->length);
  buf += BIT64SZ;
  buf = p9_pstring(buf, f->stat->name);
  buf = p9_pstring(buf, f->stat->uid);
  buf = p9_pstring(buf, f->stat->gid);
  buf = p9_pstring(buf, f->stat->muid);
  return 0;
}

int p9_compose_stat(char* data, struct p9_stat *stat, struct p9_qid *qid) {
  int len = stat->size;
  uint8_t* p = (uint8_t*)data;
  PBIT16(p, len);
  p += BIT16SZ;
  PBIT16(p, stat->type);
  p += BIT16SZ;
  PBIT32(p, stat->dev);
  p += BIT32SZ;
  
  PBIT8(p, qid->type);
  p += BIT8SZ;
  PBIT32(p, qid->vers);
  p += BIT32SZ;
  PBIT64(p, qid->path);
  p += BIT64SZ;

  PBIT32(p, stat->mode);
  p += BIT32SZ;
  PBIT32(p, stat->atime);
  p += BIT32SZ;
  PBIT32(p, stat->mtime);
  p += BIT32SZ;
  PBIT64(p, stat->length);
  p += BIT64SZ;
  p = p9_pstring(p, stat->name);
  p = p9_pstring(p, stat->uid);
  p = p9_pstring(p, stat->gid);
  p = p9_pstring(p, stat->muid);
  return p - (uint8_t*)data;
}

struct p9_stat* p9_get_stat(char *path) {
  struct p9_stat* stat;
  struct stat st;
  int fd;

  if ((fd = p9open(path, O_RDONLY)) == -1) {
    return 0;
  }

  stat = p9malloc(sizeof *stat);

  if (fstat(fd, &st) < 0) {
    fprintf(2, "[get_stat] cannot stat path: %s\n", path);
    return 0;
  }

  // TODO time, uid, gid
  stat->type = 0;
  stat->dev = st.dev;
  stat->mode = ((st.type & T_DIR) << 31) + P9_DEFPERM;
  stat->atime = 0;
  stat->mtime = 0;
  stat->length = (st.type & T_DIR) ? 0 : st.size;
  char *p = path+strlen(path);
  while(*p != '/') {
    p--;
  }
  stat->name = ++p;
  stat->uid = "nobody";
  stat->gid = "";
  stat->muid = "";
  stat->size = P9_RSTAT_DEFLEN - 2 + strlen(stat->name) + 
    strlen(stat->uid) + strlen(stat->gid) +
    strlen(stat->muid) + BIT16SZ * 4;

  close(fd);
  return stat;
}
