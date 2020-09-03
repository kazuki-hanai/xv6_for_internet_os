#include "user.h"
#include "stat.h"
#include "fcntl.h"
#include "p9.h"
#include "net/byteorder.h"

int p9_getqid(char* path, struct p9_qid* qid) {
  struct stat st;
  
  int fd;
  if ((fd = p9open(path, O_RDONLY)) == -1) {
    printf("[allocqid] cannot open: %s\n", path);
    return -1;
  }

  if (fstat(fd, &st) < 0) {
    printf("[getqid] cannot stat path: %s\n", path);
    return -1;
  }

  qid->path = (uint64_t)st.ino;
  qid->vers = 0;
  qid->type = to_qid_type(st.type);
  
  close(fd);
  return 0;
}
