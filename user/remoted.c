#include "user.h"

static void start_remoted() {
	
}

int main(int argc, char** argv) {
	
	if (fork() == 0) {
		start_remoted();
	}

	exit(0);
}
