#include "user.h"
#include "param.h"
#include "p9.h"
#include "net/byteorder.h"

struct p9_file* p9_allocfile(
  char* path,
  struct p9_filesystem* fs,
  struct p9_qid* parent
) {
  struct p9_file* file;
  file = p9malloc(sizeof *file);
  file->fs = fs;
  file->path = path;
  file->parent = 0;
  // file->parent = parent;
  file->child_num = 0;
  // TODO: make filelist
  for (int i = 0; i < 32; i++) {
    file->childs[i] = 0;
  }
  file->aux = 0;

  return file;
}

void p9_freefile(struct p9_file* file) {
  if (file->fd != -1) {
    close(file->fd);
  }
  free(file);
}
