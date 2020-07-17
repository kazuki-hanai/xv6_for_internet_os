#include "kernel/include/types.h"
#include "kernel/include/stat.h"
#include "kernel/include/fcntl.h"
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

void print_help(char *program) {
  printf("usage: %s [-lc]\n", program);
  printf("-l port: listen connection by specified port\n");
  printf("-c dist port: connect to specified distnation\n");
  printf("-t testname: execute testcode specified testname\n");
}

int get_arg(int argc, char **argv) {
  if (argc < 2) {
    return -1;
  }

  if (argv[1][0] == '-' && argv[1][1] == 'l' && argv[1][2] == 0 && argc == 3) {
    return 0;
  } else if (argv[1][0] == '-' && argv[1][1] == 'c' && argv[1][2] == 0 && argc == 4) {
    return 1;
  } else if (argv[1][0] == '-' && argv[1][1] == 't' && argv[1][2] == 0 && argc > 3) {
    return 2;
  } else  {
    return -1;
  }
}

int sock_listen(char **argv) {
  uint16 sport = atoi(argv[0]);
  int sock;
  sock = socket(SOCK_TCP);
  listen(sock, sport);
  return sock;
}

int sock_connect(char **argv) {
  uint32 raddr = get_ip(argv[0]);
  uint16 dport = atoi(argv[1]);
  int sock;
  sock = socket(SOCK_TCP);
  connect(sock, raddr, dport);
  return sock;
}

void chat(int sock) {
  while(1) {
    char rbuf[256];
    char wbuf[256];
    wbuf[0] = 0;

    if (read(sock, rbuf, 256) == -1) {
      printf("read failed! connection closed.\n");
      break;
    }
    printf("you: %s", rbuf);

    printf("me: ");
    int wsize = read(1, wbuf, 256);
    if (wbuf[0] == 0)
      break;
    if (write(sock, wbuf, wsize) == -1) {
      printf("write failed! connection closed.\n");
      break;
    }
  }
}

void send_file(int sock) {
  int fd = open("ls", O_RDONLY);
  char filebuf[25000];
  struct stat st;
  fstat(fd, &st);
  read(fd, filebuf, st.size);
  write(sock, filebuf, sizeof(filebuf));
}

void recv_file(int sock) {
  int fd = open("ls_send", O_WRONLY|O_CREATE);
  char filebuf[25000];
  read(sock, filebuf, sizeof(filebuf));
  write(fd, filebuf, sizeof(filebuf));
}

int
main(int argc, char **argv)
{
  if (argc < 2) {
    print_help(argv[0]);
    exit(1);
  }

  int op = get_arg(argc, argv);

  // Listen
  if (op == 0) {
    int sock = sock_listen(&argv[2]);
    chat(sock);
    close(sock);

  // Connect
  } else if (op == 1) {
    int sock = sock_connect(&argv[2]);
    chat(sock);
    close(sock);

  // Test
  } else if (op == 2) {
    if (strcmp(argv[2], "file") == 0) {
      int sock = sock_listen(&argv[3]);
      send_file(sock);
      recv_file(sock);
      close(sock);
    } else {
      print_help(argv[0]);
    }

  // help
  } else {
    print_help(argv[0]);
    exit(1);
  }
  exit(0);
}

