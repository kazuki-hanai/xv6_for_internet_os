#include "user.h"
#include "param.h"
#include "styx2000.h"
#include "net/byteorder.h"
#include "fcall.h"

struct styx2000_file* styx2000_allocfile(
  char* path,
  struct styx2000_filesystem* fs,
  struct styx2000_qid* parent
) {
  struct styx2000_file* file;
  file = malloc(sizeof *file);
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

void styx2000_freefile(struct styx2000_file* file) {
  if (file->fd != -1) {
    close(file->fd);
  }
  free(file);
}
