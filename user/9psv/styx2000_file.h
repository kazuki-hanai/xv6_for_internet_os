#pragma once

#include "types.h"
#include "file.h"
#include "styx2000.h"
#include "fcall.h"

struct styx2000_stat*   styx2000_get_stat(char *path);
void                    styx2000_freefile(struct styx2000_file* file);
struct styx2000_file*   styx2000_allocfile( char* path, struct styx2000_filesystem* fs, struct styx2000_qid* parent);
