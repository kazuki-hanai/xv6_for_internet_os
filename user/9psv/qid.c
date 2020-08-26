#include "user.h"
#include "stat.h"
#include "fcntl.h"
#include "p9.h"
#include "net/byteorder.h"
#include "p9_file.h"

static void freeqid(struct p9_qid* qid);

static void incqidref(struct p9_qid *qid) {
  qid->ref += 1;
}
static void decqidref(struct p9_qid *qid) {
  qid->ref -= 1;
}
static int is_referenced(struct p9_qid *qid) {
  return qid->ref != 0;
}
static inline uint8_t to_qid_type(uint16_t t) {
  uint8_t res = 0;
  if (t & T_DIR) {
    res |= P9_ODIR;
  }
  return res;
}

uint64_t p9_getqidno(char* path) {
  int fd;
  struct stat st;

  if ((fd = open(path, O_RDONLY)) == -1) {
    printf("[getqidno] cannot open: %s\n", path);
    return -1;
  }

  if (fstat(fd, &st) < 0) {
    printf("[getqidno] cannot stat path: %s\n", path);
    close(fd);
    return -1;
  }

  close(fd);
  return (uint64_t)st.ino;
}

static struct p9_qid* get_qid(struct p9_qidpool* qpool, char* path) {
  struct p9_qid *qid;
  struct stat st;
  
  int fd;
  if ((fd = open(path, O_RDONLY)) == -1) {
    printf("[allocqid] cannot open: %s\n", path);
    return 0;
  }

  qid = malloc(sizeof *qid);
  if (qid == 0) {
    printf("[get_qid] malloc failed\n");
    return 0;
  }

  if (fstat(fd, &st) < 0) {
    printf("[get_qid] cannot stat path: %s\n", path);
    return 0;
  }

  qid->pathname = path;
  qid->path = (uint64_t)st.ino;
  qid->vers = 0;
  qid->type = to_qid_type(st.type);
  qid->file = 0;
  qid->ref = 1;
  qid->qpool = qpool;
  
  close(fd);
  return qid;
}

struct p9_qidpool* p9_allocqidpool() {
  struct p9_qidpool *qpool;
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

void p9_freeqidpool(struct p9_qidpool *qpool) {
  freemap(qpool->map, (void (*)(void *))qpool->destroy);
  free(qpool);
}

struct p9_qid* p9_lookupqid(struct p9_qidpool *qpool, uint64_t qid) {
  return lookupkey(qpool->map, qid);
}

struct p9_qid* p9_removeqid(struct p9_qidpool *qpool, uint64_t qid) {
  return deletekey(qpool->map, qid);
}

struct p9_qid* p9_allocqid(
  struct p9_qidpool* qpool,
  struct p9_qid* parent,
  struct p9_filesystem* fs,
  char* path
) {
  struct p9_qid *qid;

  char *pathname = malloc(strlen(path)+1);
  strcpy(pathname, path);
  pathname[strlen(path)] = 0;

  qid = get_qid(qpool, pathname);
  
  qid->file = p9_allocfile(pathname, fs, parent);
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

static void freeqid(struct p9_qid* qid) {
  p9_freefile(qid->file);
  if (qid->pathname != 0) {
    free(qid->pathname);
  }
  free(qid);
}

int p9_get_dir(struct p9_qid* qid) {
  struct dirent de;
  struct p9_file* file      = qid->file;
  struct p9_filesystem* fs  = file->fs;
  char path[256], *p;
  int i = 0;

  strcpy(path, qid->pathname);
  p = path + strlen(qid->pathname);

  if (*(p-1) != '/') {
    *p = '/';
    p++;
  }
  *p = '\0';

  int fd;
  if ((fd = open(path, O_RDONLY)) == -1) {
    return -1;
  }
  while(read(fd, &de, sizeof(de)) == sizeof(de)){
    // TODO: current, parent directory
    if(de.inum == 0 || strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
      continue;
    
    strcpy(p, de.name);
    struct p9_qid* nextq;
    if ((nextq = p9_lookupqid(qid->qpool, p9_getqidno(path))) == 0) {
      nextq = p9_allocqid(qid->qpool, qid, fs, path);
    }
    file->childs[i] = nextq;
    i++;
  }
  file->child_num = i;
  close(fd);
  return 0;
}
