#include "user.h"
#include "types.h"
#include "param.h"
#include "stat.h"
#include "fcntl.h"
#include "styx2000.h"
#include "net/byteorder.h"
#include "fcall.h"

static void incqidref(struct styx2000_qid *qid) {
  qid->ref += 1;
}
static void decqidref(struct styx2000_qid *qid) {
  qid->ref -= 1;
}
static int is_referenced(struct styx2000_qid *qid) {
  return qid->ref != 0;
}

uint8 styx2000_to_qid_type(uint16 t) {
  uint8 res = 0;
  if (t & T_DIR) {
    res |= STYX2000_ODIR;
  }
  return res;
}

uint8 styx2000_to_xv6_mode(uint8 m) {
  uint8 res = 0;
  if (m & STYX2000_ORDWR)
    res |= O_RDWR;
  if (m & STYX2000_OWRITE) 
    res |= O_WRONLY;
  return res;
}

int styx2000_is_dir(struct styx2000_qid* qid) {
  return qid->type & STYX2000_ODIR; 
}

int styx2000_compose_stat(char* data, struct styx2000_stat *stat) {
  int len = stat->size;
  uint8* p = (uint8*)data;
  PBIT16(p, len);
  p += BIT16SZ;
  PBIT16(p, stat->type);
  p += BIT16SZ;
  PBIT32(p, stat->dev);
  p += BIT32SZ;
  
  PBIT8(p, stat->qid->type);
  p += BIT8SZ;
  PBIT32(p, stat->qid->vers);
  p += BIT32SZ;
  PBIT64(p, stat->qid->path);
  p += BIT64SZ;

  PBIT32(p, stat->mode);
  p += BIT32SZ;
  PBIT32(p, stat->atime);
  p += BIT32SZ;
  PBIT32(p, stat->mtime);
  p += BIT32SZ;
  PBIT64(p, stat->length);
  p += BIT64SZ;
  p = styx2000_pstring(p, stat->name);
  p = styx2000_pstring(p, stat->uid);
  p = styx2000_pstring(p, stat->gid);
  p = styx2000_pstring(p, stat->muid);
  return p - (uint8*)data;
}

static struct styx2000_stat* get_stat(int fd, char *path, struct styx2000_qid* qid) {
  struct styx2000_stat* stat;
  struct stat st;
  stat = malloc(sizeof *stat);

  if (fstat(fd, &st) < 0) {
    fprintf(2, "[get_stat] cannot stat path: %s\n", path);
    return 0;
  }

  // TODO time, uid, gid
  stat->type = 0;
  stat->dev = st.dev;
  stat->qid = qid;
  stat->mode = (styx2000_to_qid_type(st.type) << 24) + STYX2000_DEFPERM;
  stat->atime = 0;
  stat->mtime = 0;
  stat->length = st.size;
  stat->name = path;
  stat->uid = "guest";
  stat->gid = "guest";
  stat->muid = "";
  stat->size = STYX2000_RSTAT_DEFLEN - 2 + strlen(stat->name) + 
    strlen(stat->uid) + strlen(stat->gid) +
    strlen(stat->muid) + BIT16SZ * 4;
  return stat;
}

static struct styx2000_qid* get_qid(struct styx2000_qidpool* qpool, char* path) {
  struct styx2000_qid *qid;
  struct stat st;
  
  int fd;
  if ((fd = open(path, O_RDONLY)) == -1) {
    fprintf(2, "[allocqid] cannot open: %s\n", path);
    return 0;
  }

  qid = malloc(sizeof *qid);
  if (qid == 0) {
    fprintf(2, "[get_qid] malloc failed\n");
    return 0;
  }

  if (fstat(fd, &st) < 0) {
    fprintf(2, "[get_qid] cannot stat path: %s\n", path);
    return 0;
  }

  qid->pathname = path;
  qid->path = (uint64)st.ino;
  qid->vers = 0;
  qid->type = styx2000_to_qid_type(st.type);
  qid->file = 0;
  qid->ref = 1;
  qid->qpool = qpool;
  
  close(fd);
  return qid;
}

static void freefile(struct styx2000_file* file) {
  free(file->stat);
  free(file);
}

static void freeqid(struct styx2000_qid* qid) {
  freefile(qid->file);
  if (qid->pathname != 0) {
    free(qid->pathname);
  }
  free(qid);
}

static struct styx2000_file* allocfile(
  int fd,
  char* path,
  int mode,
  struct styx2000_filesystem* fs,
  struct styx2000_qid* parent,
  struct styx2000_qid* qid
) {
  struct styx2000_file* file;
  file = malloc(sizeof *file);
  file->fd = fd;
  file->fs = fs;
  file->path = path;
  file->stat = get_stat(file->fd, path, qid);
  file->parent = parent;
  file->child_num = 0;
  // TODO: make list
  for (int i = 0; i < 32; i++) {
    file->childs[i] = 0;
  }
  file->aux = 0;

  return file;
}

struct styx2000_qidpool* styx2000_allocqidpool() {
  struct styx2000_qidpool *qpool;
  qpool = malloc(sizeof *qpool);
  if (qpool == 0) {
    return 0;
  }
  qpool->destroy = freeqid;
  if ((qpool->map = allocmap(0)) == 0) {
    free(qpool);
    return 0;
  }
  return qpool;
}

void styx2000_freeqidpool(struct styx2000_qidpool *qpool) {
  freemap(qpool->map, (void (*)(void *))qpool->destroy);
  free(qpool);
}

uint64 styx2000_getqidno(char* path) {
  int fd;
  struct stat st;

  if ((fd = open(path, O_RDWR)) == -1) {
    fprintf(2, "[getqidno] cannot open: %s\n", path);
    return -1;
  }

  if (fstat(fd, &st) < 0) {
    fprintf(2, "[getqidno] cannot stat path: %s\n", path);
    close(fd);
    return -1;
  }

  close(fd);
  return (uint64)st.ino;
}

struct styx2000_qid* styx2000_lookupqid(struct styx2000_qidpool *qpool, uint64 qid) {
  return lookupkey(qpool->map, qid);
}

struct styx2000_qid* styx2000_removeqid(struct styx2000_qidpool *qpool, uint64 qid) {
  return deletekey(qpool->map, qid);
}

struct styx2000_qid* styx2000_allocqid(
  struct styx2000_qidpool* qpool,
  struct styx2000_qid* parent,
  char* path
) {
  struct styx2000_qid *qid;

  char *pathname = malloc(strlen(path));
  strcpy(pathname, path);

  qid = get_qid(qpool, pathname);
  
  int mode, fd;
  mode = styx2000_is_dir(qid) ? O_RDONLY : O_RDWR;
  if ((fd = open(pathname, mode)) == -1) {
    fprintf(2, "[allocqid] cannot open: %s\n", path);
    return 0;
  }

  qid->file = allocfile(fd, pathname, mode, &qpool->srv->fs, parent, qid);
  qid->ref = 1;
  qid->qpool = qpool;
  qid->inc = incqidref;
  qid->dec = decqidref;
  qid->is_referenced = is_referenced;
  if (caninsertkey(qpool->map, qid->path, qid) == 0) {
    freeqid(qid);
    return 0;
  }
  return qid;
}

int styx2000_get_dir(struct styx2000_qid* qid) {
  struct dirent de;
  struct styx2000_file* file = qid->file;
  int i = 0;
  while(read(file->fd, &de, sizeof(de)) == sizeof(de)){
    if(de.inum == 0)
      continue;
    struct styx2000_qid* nextq;
    if ((nextq = styx2000_lookupqid(qid->qpool, styx2000_getqidno(de.name))) == 0) {
      nextq = styx2000_allocqid(qid->qpool, qid, de.name);
    }
    file->childs[i] = nextq;
    i++;
  }
  file->child_num = i;
  return 0;
}
