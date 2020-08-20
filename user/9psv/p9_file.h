#pragma once

#include "p9.h"

struct p9_stat*   p9_get_stat(char *path);
void                    p9_freefile(struct p9_file* file);
struct p9_file*   p9_allocfile( char* path, struct p9_filesystem* fs, struct p9_qid* parent);
