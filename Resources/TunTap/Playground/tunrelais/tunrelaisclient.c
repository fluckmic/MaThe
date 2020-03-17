#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <stdarg.h>

// buffer for reading from tun/tap interface, must be >= 1500
#define BUFFER_SIZE 2000

#define SERVER_IP "192.168.1.45"
#define SERVER_PORT 55555

#define NAME_VIF_1 "tun33"
#define NAME_VIF_2 "tun34"

#define DEBUG 0

int debug = DEBUG;

#define max(a,b) ((a)>(b) ? (a):(b))

struct ip_packet_header
{
  uint8_t        version;
  uint8_t        ihl;
  uint8_t        type_of_service;
  uint16_t       total_length;
  struct in_addr source;
  struct in_addr destination;
} ip_header;

struct tcp_packet_header
{
  unsigned short source_port;
  unsigned short destination_port;
} tcp_header;

struct shila_packet_header
{
  struct ip_packet_header  ip;
  struct tcp_packet_header tcp;
} shila_header;

/**************************************************************************
 * parse_packet:                                                          *
 **************************************************************************/
int parse_packet(char *buf, int n, struct shila_packet_header* shila_packet)
{
  // Parse the ip header
  //shila_packet->ip.source.s_addr      = (unsigned long) atol(&buf[12]);
  //shila_packet->ip.destination.s_addr = (unsigned long) atol(&buf[16]);
  //printf("%lu\n", shila_packet->ip.source.s_addr);
  //printf("%s\n", inet_ntoa(shila_packet->ip.source));
  //printf("%lu\n", shila_packet->ip.destination.s_addr);
  //printf("%s\n\n", inet_ntoa(shila_packet->ip.destination));
  return 0;
}

/**************************************************************************
 * tun_alloc: allocates or reconnects to a tun/tap device. The caller     *
 *            must reserve enough space in *dev.                          *
 **************************************************************************/
int tun_alloc(char *dev, int flags) {

  struct ifreq ifr;
  int fd, err;
  char *clonedev = "/dev/net/tun";

  if( (fd = open(clonedev , O_RDWR)) < 0 ) {
    perror("tunrelais.c - Opening /dev/net/tun.");
    return fd;
  }

  memset(&ifr, 0, sizeof(ifr));

  ifr.ifr_flags = flags;

  if (*dev) {
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);
  }

  if( (err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0 ) {
    perror("tunrelais.c - ioctl(TUNSETIFF).");
    close(fd);
    return err;
  }

  strcpy(dev, ifr.ifr_name);

  return fd;
}

/**************************************************************************
 * cread: read routine that checks for errors and exits if an error is    *
 *        returned.                                                       *
 **************************************************************************/
int cread(int fd, char *buf, int n){

  int nread;

  if((nread=read(fd, buf, n)) < 0){
    perror("tunrelais.c - Reading data.");
    exit(1);
  }
  return nread;
}

/**************************************************************************
 * cwrite: write routine that checks for errors and exits if an error is  *
 *         returned.                                                      *
 **************************************************************************/
int cwrite(int fd, char *buf, int n){

  int nwrite;

  if((nwrite=write(fd, buf, n)) < 0){
    perror("tunrelais.c - Writing data.");
    exit(1);
  }
  return nwrite;
}

/**************************************************************************
 * read_n: ensures we read exactly n bytes, and puts them into "buf".     *
 *         (unless EOF, of course)                                        *
 **************************************************************************/
int read_n(int fd, char *buf, int n) {

  int nread, left = n;

  while(left > 0) {
    if ((nread = cread(fd, buf, left)) == 0){
      return 0 ;
    }else {
      left -= nread;
      buf += nread;
    }
  }
  return n;
}

/**************************************************************************
 * do_debug: prints debugging stuff (doh!)                                *
 **************************************************************************/
void do_debug(char *msg, ...){

  va_list argp;

  if(debug) {
	va_start(argp, msg);
	vfprintf(stderr, msg, argp);
	va_end(argp);
  }
}

/**************************************************************************
 * my_err: prints custom error messages on stderr.                        *
 **************************************************************************/
void my_err(char *msg, ...) {

  va_list argp;

  va_start(argp, msg);
  vfprintf(stderr, msg, argp);
  va_end(argp);
}

int main(int argc, char *argv[])
{
  char buffer[BUFFER_SIZE];

  char name_vif_1[] = NAME_VIF_1;
  char name_vif_2[] = NAME_VIF_2;

  int fd_vif_1, fd_vif_2, fd_socket, fd_net, max_fd;

  struct sockaddr_in address_local, address_remote;

  uint16_t nread, nwrite, plength;

  // Initialize the tun interfaces
  if ( (fd_vif_1 = tun_alloc(name_vif_1, IFF_TUN | IFF_NO_PI)) < 0 )
  {
    my_err("tunrelais.c - Error connecting to tun/tap interface %s!\n", name_vif_1);
    exit(1);
  }
  do_debug("tunrelais.c - Successfully connected to interface %s.\n", name_vif_1);

  if ( (fd_vif_2 = tun_alloc(name_vif_2, IFF_TUN | IFF_NO_PI)) < 0 )
  {
    my_err("tunrelais.c - Error connecting to tun/tap interface %s!\n", name_vif_2);
    exit(1);
  }
  do_debug("tunrelais.c - Successfully connected to interface %s.\n", name_vif_2);

  // Create the socket for the sub level connection
  if ( (fd_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("tunrelais.c - socket().");
    exit(1);
  }

  memset(&address_remote, 0, sizeof(address_remote));
  address_remote.sin_family      = AF_INET;
  address_remote.sin_addr.s_addr = inet_addr(SERVER_IP);
  address_remote.sin_port        = htons(SERVER_PORT);

  if (connect(fd_socket, (struct sockaddr*) &address_remote, sizeof(address_remote)) < 0)
  {
    perror("tunrelais.c - connect().");
    exit(1);
  }

  fd_net = fd_socket;
  do_debug("tunrelais.c - CLIENT: Connected to server %s.\n", inet_ntoa(address_remote.sin_addr));

  // use select() to handle two descriptors at once
  max_fd = max(max(fd_vif_1, fd_vif_2), fd_net);

  while(1)
  {
    int ret;
    fd_set rd_set;

    FD_ZERO(&rd_set);
    FD_SET(fd_vif_1, &rd_set); FD_SET(fd_vif_2, &rd_set); FD_SET(fd_net, &rd_set);

    ret = select(max_fd + 1, &rd_set, NULL, NULL, NULL);

    if (ret < 0 && errno == EINTR) { continue; }
    if (ret < 0) { perror("tunrelais.c - select()."); exit(1); }

    if(FD_ISSET(fd_vif_1, &rd_set))
    {
      nread = cread(fd_vif_1, buffer, BUFFER_SIZE);
      do_debug("tunrelais.c - Read %d bytes from %s.\n", nread, name_vif_1);

      // write length + packet
      plength = htons(nread);
      nwrite = cwrite(fd_net, (char *)&plength, sizeof(plength));
      nwrite = cwrite(fd_net, buffer, nread);
      do_debug("tunrelais.c - Forward %d bytes to the network.\n", nwrite);
    }

    if(FD_ISSET(fd_vif_2, &rd_set))
    {
      nread = cread(fd_vif_2, buffer, BUFFER_SIZE);
      do_debug("tunrelais.c - Read %d bytes from %s.\n", nread, name_vif_2);

      // write length + packet
      plength = htons(nread);
      nwrite = cwrite(fd_net, (char *)&plength, sizeof(plength));
      nwrite = cwrite(fd_net, buffer, nread);
      do_debug("tunrelais.c - Forward %d bytes to the network.\n", nwrite);
    }

    if(FD_ISSET(fd_net, &rd_set))
    {
      // Read length
      nread = read_n(fd_net, (char *)&plength, sizeof(plength));
      if(nread == 0) { break;  } // ctrl-c at the other end

      // read packet
      nread = read_n(fd_net, buffer, ntohs(plength));
      do_debug("tunrelais.c - Read %d bytes from the network.\n", nread);

      // parse the packet
      struct shila_packet_header shila_packet;
      memset(&shila_packet, 0, sizeof(shila_packet));
      ret = parse_packet(buffer, nread, &shila_packet);

      // now buffer[] contains a full packet or frame, write it into the tun/tap interface
      nwrite = cwrite(fd_vif_1, buffer, nread);
      do_debug("tunrelais.c - Written %d bytes to %s.\n", nwrite, name_vif_1);
    }
  }

  return(0);
}
