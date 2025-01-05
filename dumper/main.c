#include <sys/errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>

int main(int argc, char *argv[]) {
  char bytes[32000];
  ssize_t count;

  if (argc != 2) {
    printf("Usage %s: <tty path>\n", argv[0]);
    exit(1);
  }

  printf("Opening %s\n", argv[1]);

  int fd = open(argv[1], O_RDWR | O_NOCTTY | O_SYNC);
//  fcntl(fd, F_SETFL, 0);    /* set blocking mode */
//
//  struct termios options;
//
//  if (tcgetattr(fd,&options) < 0) {
//      printf("Error getting attributes: %s\n",strerror(errno));
//      return -1;
//  }
//  cfmakeraw(&options);
//  cfsetspeed(&options, B38400); 
//
//  options.c_cflag &= ~CSTOPB;         // one stop bit
//  options.c_cflag |= CS8;         /* 8-bit characters */
//  options.c_cflag &= ~PARENB;
//  
//  options.c_cflag |= CLOCAL;          // always set
//  options.c_cflag |= CREAD;           // always set
//  options.c_cflag &= ~CSIZE;
//                                      //
//  options.c_iflag |= ICRNL;       /* CR is a line terminator */
//  options.c_iflag |= IGNPAR;      // Ignore parity errors
//                                   //
//  options.c_cflag &= ~CRTSCTS;         // enable rts/cts
//  options.c_iflag &= ~(IXON | IXOFF | IXANY);
//
//  options.c_lflag |= ICANON;
//  options.c_lflag &= ~(ECHO | ECHOE | ISIG);
//  options.c_oflag |= OPOST;
//
//  if (tcsetattr(fd,TCSANOW,&options) != 0) {
//      printf("Error setting attributes: %s\n",strerror(errno));
//      return -1;
//  }
//
//  tcflush(fd,TCIFLUSH);              // flush input
//
  if (fd < 0) {
    printf("Could not open '%s'!\n", argv[1]);
    exit(1);
  }

  while (1) {
    count = read(fd, bytes, 32000);
    write(1, bytes, count);
  }
}
