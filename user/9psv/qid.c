#include "user.h"
#include "types.h"
#include "param.h"
#include "stat.h"
#include "fcntl.h"
#include "styx2000.h"
#include "net/byteorder.h"
#include "fcall.h"
#include "styx2000_file.h"

static void freeqid(struct styx2000_qid* qid);

static void incqidref(struct styx2000_qid *qid) {
  qid->ref += 1;
}
static void decqidref(struct styx2000_qid *qid) {
  qid->ref -= 1;
}
static int is_referenced(struct styx2000_qid *qid) {
  return qid->ref != 0;
}

static inline uint8_t to_qid_type(uint16_t t) {
  uint8_t res = 0;
  if (t & T_DIR) {
    res |= STYX2000_ODIR;
  }
  return res;
}

uint64_t styx2000_getqidno(char* path) {
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

static struct styx2000_qid* get_qid(struct styx2000_qidpool* qpool, char* path) {
  struct styx2000_qid *qid;
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

struct styx2000_qid* styx2000_lookupqid(struct styx2000_qidpool *qpool, uint64_t qid) {
  return lookupkey(qpool->map, qid);
}

struct styx2000_qid* styx2000_removeqid(struct styx2000_qidpool *qpool, uint64_t qid) {
  return deletekey(qpool->map, qid);
}

struct styx2000_qid* styx2000_allocqid(
  struct styx2000_qidpool* qpool,
  struct styx2000_qid* parent,
  struct styx2000_filesystem* fs,
  char* path
) {
  struct styx2000_qid *qid;

  char *pathname = malloc(strlen(path)+1);
  strcpy(pathname, path);
  pathname[strlen(path)] = 0;

  qid = get_qid(qpool, pathname);
  
  qid->file = styx2000_allocfile(pathname, fs, parent);
  qid->ref = 1;
  qid->qpool = qpool;
  qid->inc = incqidref;
  qid->dec = decqidref;
  qid->is_referenced = is_referenced;
  if (caninsertkey(qpool->map, qid->path, qid) == 0) {
  printf("kore\n");
    freeqid(qid);
    return 0;
  }
  return qid;
}

static void freeqid(struct styx2000_qid* qid) {
  styx2000_freefile(qid->file);
  if (qid->pathname != 0) {
    free(qid->pathname);
  }
  free(qid);
}

int styx2000_get_dir(struct styx2000_qid* qid, struct styx2000_filesystem* fs) {
  struct dirent de;
  struct styx2000_file* file = qid->file;
  char path[256], *p;
  int i = 0;

  strcpy(path, qid->pathname);
  p = path + strlen(qid->pathname);

  int fd;
  if ((fd = open(path, O_RDONLY)) == -1) {
    return -1;
  }
  while(read(fd, &de, sizeof(de)) == sizeof(de)){
    if(de.inum == 0 || de.inum == 1)
      continue;
    
    strcpy(p, de.name);
    struct styx2000_qid* nextq;
    if ((nextq = styx2000_lookupqid(qid->qpool, styx2000_getqidno(path))) == 0) {
      nextq = styx2000_allocqid(qid->qpool, qid, fs, path);
    }
    file->childs[i] = nextq;
    i++;
  }
  file->child_num = i;
  close(fd);
  return 0;
}
