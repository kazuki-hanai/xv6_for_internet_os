#include "kernel/include/types.h"
#include "kernel/include/stat.h"
#include "kernel/include/net/sock_cb.h"
#include "user/user.h"

uint32 get_ip(char *ip) {
  int len = strlen(ip);
  int b = 0;
  uint32 res = 0;
  for (int i = 0; i < len; i++) {
    if (ip[i] == '.') {
      res <<= 8;
      res += b;
      b = 0;
    } else {
      if('0' <= ip[i] && ip[i] <= '9')
        b = b*10 + ip[i] - '0'; 
      else
        return 0;
    }
  }
  res <<= 8;
  res += b;
  return res;
}

int
main(int argc, char **argv)
{
  if (argc < 2) {
    printf("usage: %s port\n", argv[0]);
    exit(1);
  }
  uint16 sport = atoi(argv[1]);
  int sock;

  sock = socket(SOCK_TCP);
  listen(sock, sport);

  while(1) {
    char rbuf[256];
    char wbuf[256];
    wbuf[0] = 0;

    read(sock, rbuf, 256);
    printf("%s\n", rbuf);
    int wsize = read(1, wbuf, 256);

    if (wbuf[0] == 0)
      break;
    write(sock, wbuf, wsize);

  }

  close(sock);
  exit(0);
}

