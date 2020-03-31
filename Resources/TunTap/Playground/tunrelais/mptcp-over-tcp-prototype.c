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

#define SERVER 0
#define SERVER_IP "192.168.1.58"
#define SERVER_PORT 55555

#define VIF_1_IP "10.0.1.1"
#define VIF_2_IP "10.0.2.1"
#define VIF_3_IP "10.0.3.1"
#define VIF_I_IP "10.7.0.9"

#define NAME_VIF_1 "tun33"
#define NAME_VIF_2 "tun34"
#define NAME_VIF_3 "tun35"
#define NAME_VIF_I "tun66"

#define DEBUG         0
#define DEBUG_PARSING 0

int debug         = DEBUG;
int debug_parsing = DEBUG_PARSING;

#define max(a,b) ((a)>(b) ? (a):(b))

struct ip_packet_header
{
  uint8_t        version;
  uint8_t        ihl;
  uint8_t        type_of_service;
  uint16_t       total_length;
  uint16_t       identification;
  uint8_t        ttl;
  uint8_t        protocol;
  uint16_t       header_checksum;
  struct in_addr source;
  struct in_addr destination;
} ip_header;

struct tcp_packet_header
{
  uint16_t       source_port;
  uint16_t       destination_port;
} tcp_header;

struct shila_packet_header
{
  char                      add_info[32];
  struct ip_packet_header   ip;
  struct tcp_packet_header  tcp;
} shila_header;

int parse_packet(char *buf, int n, struct shila_packet_header* shila_packet)
{
  // It is assumed that buffer contains the data in network byte order (big endian)
  shila_packet->ip.version            = buf[0] >> 4;
  shila_packet->ip.ihl                = buf[0] & 0x0f;
  shila_packet->ip.type_of_service    = buf[1];
  shila_packet->ip.total_length       = *(uint16_t*) (&buf[2]);
  shila_packet->ip.identification     = *(uint16_t*) (&buf[4]);
  shila_packet->ip.ttl                = buf[8];
  shila_packet->ip.protocol           = buf[9];
  shila_packet->ip.header_checksum    = *(uint16_t*) (&buf[10]);
  shila_packet->ip.source.s_addr      = *(unsigned long*)(&buf[12]);
  shila_packet->ip.destination.s_addr = *(unsigned long*)(&buf[16]);

  // Check if there are extra options.
  if(shila_packet->ip.ihl > 5)
  {
    if(buf[20] ==  7) // Loose Source and Record Route
    {
      if(buf[21] == 39)
      {
        if(buf[22] ==  8) // One hop.
        {
          for(int i = 0; i < 32; i++)
          {
            shila_packet->add_info[i] = buf[27 + i];
          }
        }
      }
    }
  }

  const uint8_t tcp_header_base = (shila_packet->ip.ihl * 4);

  shila_packet->tcp.source_port       = *(uint16_t*) (&buf[tcp_header_base]);
  shila_packet->tcp.destination_port  = *(uint16_t*) (&buf[tcp_header_base + 2]);

  return 0;
}

void print_packet(struct shila_packet_header* shila_packet)
{

  printf("\n");
  printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
  printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
  printf("Version: %i\n", shila_packet->ip.version);
  printf("IHL: %i\n", shila_packet->ip.ihl);
  printf("Type of service: %i\n", shila_packet->ip.type_of_service);
  printf("Total length: %u\n", ntohs(shila_packet->ip.total_length));
  printf("Identification: %u\n", ntohs(shila_packet->ip.identification));
  printf("TTL: %i\n", shila_packet->ip.ttl);
  printf("Protocol: %i\n", shila_packet->ip.protocol);
  printf("Header checksum: %x\n", ntohs(shila_packet->ip.header_checksum));
  printf("Source: %s\n", inet_ntoa(shila_packet->ip.source));
  printf("Destination: %s\n", inet_ntoa(shila_packet->ip.destination));

  printf("From app: ");
  for(int i = 0; i < 32; i++)
  {
    printf("%d ", shila_packet->add_info[i]);
    if(i % 10 == 0 && i > 0) printf("\n          ");
  }
  printf("\n");

  printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
  printf("\n");

  printf("Source port: %u\n", ntohs(shila_packet->tcp.source_port));
  printf("Destination port: %u\n", ntohs(shila_packet->tcp.destination_port));
  printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
  printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");

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
    perror("mptcp-over-tcp-prototype.c - Opening /dev/net/tun.");
    return fd;
  }

  memset(&ifr, 0, sizeof(ifr));

  ifr.ifr_flags = flags;

  if (*dev) {
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);
  }

  if( (err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0 ) {
    perror("mptcp-over-tcp-prototype.c - ioctl(TUNSETIFF).");
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
    perror("mptcp-over-tcp-prototype.c - Reading data.");
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
    perror("mptcp-over-tcp-prototype.c - Writing data.");
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
  char name_vif_3[] = NAME_VIF_3;
  char name_vif_i[] = NAME_VIF_I;

  int fd_vif_1, fd_vif_2, fd_vif_3, fd_vif_i, fd_socket, fd_net, max_fd;
  int optval = 1;

  struct sockaddr_in address_local, address_remote;

  uint16_t nread, nwrite, plength;
  socklen_t len_remote;

  // Initialize the tun interfaces
    if ( (fd_vif_1 = tun_alloc(name_vif_1, IFF_TUN | IFF_NO_PI)) < 0 )
    {
      my_err("mptcp-over-tcp-prototype.c - Error connecting to tun/tap interface %s!\n", name_vif_1);
      exit(1);
    }
    do_debug("mptcp-over-tcp-prototype.c - Successfully connected to interface %s.\n", name_vif_1);

    if ( (fd_vif_2 = tun_alloc(name_vif_2, IFF_TUN | IFF_NO_PI)) < 0 )
    {
      my_err("mptcp-over-tcp-prototype.c - Error connecting to tun/tap interface %s!\n", name_vif_2);
      exit(1);
    }
    do_debug("mptcp-over-tcp-prototype.c - Successfully connected to interface %s.\n", name_vif_2);


    if ( (fd_vif_3 = tun_alloc(name_vif_3, IFF_TUN | IFF_NO_PI)) < 0 )
    {
      my_err("mptcp-over-tcp-prototype.c - Error connecting to tun/tap interface %s!\n", name_vif_3);
      exit(1);
    }
    do_debug("mptcp-over-tcp-prototype.c - Successfully connected to interface %s.\n", name_vif_3);


    if ( (fd_vif_i = tun_alloc(name_vif_i, IFF_TUN | IFF_NO_PI)) < 0 )
    {
      my_err("mptcp-over-tcp-prototype.c - Error connecting to tun/tap interface %s!\n", name_vif_i);
      exit(1);
    }
    do_debug("mptcp-over-tcp-prototype.c - Successfully connected to interface %s.\n", name_vif_i);

  // Create the socket for the sub level connection
  if ( (fd_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("mptcp-over-tcp-prototype.c - socket().");
    exit(1);
  }

  if(SERVER)
  {
    printf("mptcp-over-tcp-prototype.c - Running as SERVER.\n");

    /* avoid EADDRINUSE error on bind() */
    if(setsockopt(fd_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval)) < 0) {
      perror("mptcp-over-tcp-prototype.c - setsockopt()");
      exit(1);
    }

    memset(&address_local, 0, sizeof(address_local));
    address_local.sin_family = AF_INET;
    address_local.sin_addr.s_addr = htonl(INADDR_ANY);
    address_local.sin_port = htons(SERVER_PORT);
    if (bind(fd_socket, (struct sockaddr*) &address_local, sizeof(address_local)) < 0) {
      perror("mptcp-over-tcp-prototype.c - bind()");
      exit(1);
    }

    if (listen(fd_socket, 5) < 0) {
      perror("mptcp-over-tcp-prototype.c - listen()");
      exit(1);
    }

    /* wait for connection request */
    len_remote = sizeof(address_remote);
    memset(&address_remote, 0, len_remote);
    if ((fd_net = accept(fd_socket, (struct sockaddr*)&address_remote, &len_remote)) < 0) {
      perror("mptcp-over-tcp-prototype.c - accept()");
      exit(1);
    }

    printf("mptcp-over-tcp-prototype.c - SERVER: Client connected from %s\n", inet_ntoa(address_remote.sin_addr));
  }
  else
  {
    printf("mptcp-over-tcp-prototype.c - Running as CLIENT.\n");

    memset(&address_remote, 0, sizeof(address_remote));
    address_remote.sin_family      = AF_INET;
    address_remote.sin_addr.s_addr = inet_addr(SERVER_IP);
    address_remote.sin_port        = htons(SERVER_PORT);

    if (connect(fd_socket, (struct sockaddr*) &address_remote, sizeof(address_remote)) < 0)
    {
      perror("mptcp-over-tcp-prototype.c - connect().");
      exit(1);
    }

    fd_net = fd_socket;
    printf("mptcp-over-tcp-prototype.c - CLIENT: Connected to server %s.\n", inet_ntoa(address_remote.sin_addr));
  }

  max_fd = max(max(max(max(fd_vif_1, fd_vif_2), fd_vif_3), fd_vif_i), fd_net);
  //max_fd = max(max(max(fd_vif_1, fd_vif_2), fd_vif_i), fd_net);

  while(1)
  {
      int from_app = 0;
      int ret;
      fd_set rd_set;

      FD_ZERO(&rd_set);
      FD_SET(fd_vif_1, &rd_set); FD_SET(fd_vif_2, &rd_set); FD_SET(fd_net, &rd_set); FD_SET(fd_vif_i, &rd_set);
      FD_SET(fd_vif_3, &rd_set);

      ret = select(max_fd + 1, &rd_set, NULL, NULL, NULL);

      if (ret < 0 && errno == EINTR) { continue; }
      if (ret < 0) { perror("mptcp-over-tcp-prototype.c - select()."); exit(1); }

      if(FD_ISSET(fd_vif_1, &rd_set))
      {
        // Data came through app.
        from_app = 1;
        nread = cread(fd_vif_1, buffer, BUFFER_SIZE);
        do_debug("mptcp-over-tcp-prototype.c - Read %d bytes from %s.\n", nread, name_vif_1);
      }
      else if(FD_ISSET(fd_vif_2, &rd_set))
      {
        from_app = 1;
        nread = cread(fd_vif_2, buffer, BUFFER_SIZE);
        do_debug("mptcp-over-tcp-prototype.c - Read %d bytes from %s.\n", nread, name_vif_2);
      }
      else if(FD_ISSET(fd_vif_3, &rd_set))
      {
        from_app = 1;
        nread = cread(fd_vif_3, buffer, BUFFER_SIZE);
        do_debug("mptcp-over-tcp-prototype.c - Read %d bytes from %s.\n", nread, name_vif_3);
      }
      else if(FD_ISSET(fd_vif_i, &rd_set))
      {
        from_app = 1;
        nread = cread(fd_vif_i, buffer, BUFFER_SIZE);
        printf("mptcp-over-tcp-prototype.c - Read %d bytes from %s.\n", nread, name_vif_i);
      }
      else if(FD_ISSET(fd_net, &rd_set))
      {
        // Read length
        nread = read_n(fd_net, (char *)&plength, sizeof(plength));
        if(nread == 0) { break;  } // ctrl-c at the other end

        // read packet
        nread = read_n(fd_net, buffer, ntohs(plength));
        do_debug("mptcp-over-tcp-prototype.c - Read %d bytes from the network.\n", nread);

        // parse the packet
        struct shila_packet_header shila_packet;
        memset(&shila_packet, 0, sizeof(shila_packet));
        ret = parse_packet(buffer, nread, &shila_packet);

        if(shila_packet.ip.destination.s_addr == inet_addr(VIF_1_IP))
        {
          // now buffer[] contains a full packet or frame, write it into the tun/tap interface
          nwrite = cwrite(fd_vif_1, buffer, nread);
          do_debug("mptcp-over-tcp-prototype.c - Written %d bytes to %s.\n", nwrite, name_vif_1);
        }
        else if(shila_packet.ip.destination.s_addr == inet_addr(VIF_2_IP))
        {
          // now buffer[] contains a full packet or frame, write it into the tun/tap interface
          nwrite = cwrite(fd_vif_2, buffer, nread);
          do_debug("mptcp-over-tcp-prototype.c - Written %d bytes to %s.\n", nwrite, name_vif_2);
        }
        else if(shila_packet.ip.destination.s_addr == inet_addr(VIF_3_IP))
        {
          // now buffer[] contains a full packet or frame, write it into the tun/tap interface
          nwrite = cwrite(fd_vif_3, buffer, nread);
          do_debug("mptcp-over-tcp-prototype.c - Written %d bytes to %s.\n", nwrite, name_vif_3);
        }
        else if(shila_packet.ip.destination.s_addr == inet_addr(VIF_I_IP))
        {
          // now buffer[] contains a full packet or frame, write it into the tun/tap interface
          nwrite = cwrite(fd_vif_i, buffer, nread);
          do_debug("mptcp-over-tcp-prototype.c - Written %d bytes to %s.\n", nwrite, name_vif_i);
        }
      }

      if(from_app == 1)
      {
        // parse the packet
        struct shila_packet_header shila_packet;
        memset(&shila_packet, 0, sizeof(shila_packet));
        ret = parse_packet(buffer, nread, &shila_packet);

        // print it
        if(DEBUG_PARSING)
        {
          print_packet(&shila_packet);
        }

        // write length + packet
        plength = htons(nread);
        nwrite = cwrite(fd_net, (char *)&plength, sizeof(plength));
        nwrite = cwrite(fd_net, buffer, nread);
        do_debug("mptcp-over-tcp-prototype.c - Forward %d bytes to the network.\n", nwrite);
      }
    }

  return(0);
}
