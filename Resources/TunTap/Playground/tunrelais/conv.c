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
    if(!buf[20] ==  7) // Loose Source and Record Route
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
    printf("%d ", shila_packet->add_info[i]);
  printf("\n");

  printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
  printf("\n");

  const uint8_t tcp_header_base = (shila_packet->ip.ihl * 4);

  shila_packet->tcp.source_port       = *(uint16_t*) (&buf[tcp_header_base]);
  shila_packet->tcp.destination_port  = *(uint16_t*) (&buf[tcp_header_base + 2]);

  printf("Source port: %u\n", ntohs(shila_packet->tcp.source_port));
  printf("Destination port: %u\n", ntohs(shila_packet->tcp.destination_port));
  printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
  printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");

  return 0;
}

int main(int argc, char *argv[])
{

  char buffer[] = {  0x4f,0x00,0x00,0x75,0xc6,0xc7,0x40,0x00,0x40,0x06,0x3b,0x7f,0x0a,0x00,0x02,0x01 \
                    ,0x0a,0x00,0x01,0x01,0x07,0x27,0x10,0x0a,0x00,0x01,0x01,0x0a,0x00,0x02,0x01,0x0a \
                    ,0x00,0x02,0x01,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15 \
                    ,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x00,0x0a,0xa7,0x0e,0x34 \
                    ,0x11,0x4f,0x7f,0xdb,0x4d,0xa6,0xec,0x73,0xd0,0x18,0x01,0xf7,0xb9,0x70,0x00,0x00 \
                    ,0x01,0x01,0x08,0x0a,0x24,0x53,0x1e,0xe8,0x61,0x30,0xe1,0x52,0x1e,0x14,0x20,0x05 \
                    ,0xe4,0x16,0xa1,0x28,0x19,0x12,0x3b,0xfe,0x00,0x00,0x00,0x06,0x00,0x05,0x13,0x0a \
                    ,0x50,0x6f,0x6e,0x67,0x00};

  // parse the packet
  struct shila_packet_header shila_packet;
  memset(&shila_packet, 0, sizeof(shila_packet));
  int ret = parse_packet(buffer, sizeof(buffer), &shila_packet);

}
