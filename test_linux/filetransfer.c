#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/types.h>

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("usage: %s [send|recv] filename", argv[0]);
    return;
  }

  struct sockaddr_in addr;

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    perror("error socket");
    exit(1);
  }
  memset(&addr, 0, sizeof(addr));

  addr.sin_port = htons(2000);
  inet_aton("192.168.22.2", &addr.sin_addr.s_addr);

  printf("connecting...\n");
  if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("error connecting");
    exit(1);
  }

  char buf[1500];
  ssize_t rsize;
  if (strcmp(argv[1], "send") == 0) {
    int fd = open(argv[2], O_CREAT|O_WRONLY);
    printf("sending...\n");
    while(rsize = read(sock, buf, sizeof(buf))) {
      write(fd, buf, rsize);
    }
  } else if (strcmp(argv[1], "recv") == 0) {
    int fd = open(argv[2], O_RDONLY);
    printf("recieving...\n");
    while(rsize = read(fd, buf, sizeof(buf))) {
      write(sock, buf, rsize);
    }
  } else {
    printf("usage: %s [-l|-s]", argv[0]);
    return;
  }

  printf("complete!\n");
  return 0;
}
