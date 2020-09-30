#include "user.h"
#include "fcntl.h"
#include "p9.h"
#include "net/byteorder.h"

struct p9_file* p9_allocfile(char* path, struct p9_filesystem* fs) {
	struct p9_file* file;
	file = p9malloc(sizeof *file);
	file->fs = fs;
	file->path = p9malloc(strlen(path)+1);
	strcpy(file->path, path);
	file->path[strlen(path)] = 0;
	file->child_num = 0;
	// TODO: make filelist
	for (int i = 0; i < 32; i++) {
		file->childs[i] = 0;
	}
	file->aux = 0;

	return file;
}

static void freechild(struct p9_file* file) {
	if (file->child_num > 0) {
		for (int i = 0; i < file->child_num; i++) {
			free(file->childs[i]);
		}
	}
	file->child_num = 0;
}

void p9_freefile(struct p9_file* file) {
	freechild(file);
	free(file->path);
	free(file);
}

int p9_getdir(struct p9_file* file) {
	struct dirent de;
	char path[256], *p;
	int i = 0;

	int plen = strlen(file->path);
	strcpy(path, file->path);
	p = path + plen;

	if (*(p-1) != '/') {
		*p = '/';
		p++;
		plen += 1;
	}
	*p = '\0';

	int fd;
	if ((fd = p9open(path, O_RDONLY)) == -1) {
		return -1;
	}

	freechild(file);

	while(read(fd, &de, sizeof(de)) == sizeof(de)){
		// TODO: fix to show current/parent directories properly
		if(de.inum == 0)
			continue;
		
		char* child = p9malloc(strlen(de.name) + plen);
		strcpy(child, path);
		strcpy(child+plen, de.name);
		file->childs[i] = child;
		i++;
	}
	file->child_num = i;
	close(fd);
	return 0;
}
