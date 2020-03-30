/**************************************************************************
 * tcpserver.c                                                            *
 *************************************************************************/

 #include <netdb.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <sys/socket.h>
 #include <sys/types.h>
 #include <arpa/inet.h>
 #include <unistd.h>
 #include <netinet/in.h>
 #include <errno.h>

 #define IP_SERVER "10.7.0.9"
 #define PORT_SERVER 2727
 #define BUFFER_SIZE 1000

 /*
 struct ip_option_header
 {
   uint8_t type;
   uint8_t length;
   uint8_t pointer;
   uint8_t padding;
   uint8_t route_data[36];
 } option_data;
 */

void communicator(int fd_socket)
{
  printf("tcpserver.c - Starting endless data exchange.\n");

  char input_buffer[BUFFER_SIZE];
  char output_buffer[] = "00 01 02 03 04 05 06 07 08 09 \
                          10 11 12 13 14 15 16 17 18 19 \
                          20 21 22 23 24 25 26 27 28 29 \
                          30 31 32 33 34 35 36 37 38 39 \
                          40 41 42 43 44 45 46 47 48 49 \
                          50 51 52 53 54 55 56 57 58 59 \
                          60 61 62 63 64 65 66 67 68 69 \
                          70 71 72 73 74 75 76 77 78 79 \
                          80 81 82 83 84 85 86 87 88 89 \
                          90 91 92 93 94 95 96 97 98 99";

  while(1)
  {
    bzero(input_buffer, sizeof(input_buffer));
    read(fd_socket, input_buffer, sizeof(input_buffer));
    sleep(10);
    write(fd_socket, output_buffer, sizeof(output_buffer));
  }
}

int main(int argc, char *argv[])
{
  int fd_socket, fd_connection, length;
  struct sockaddr_in addr_server, addr_client;

  // Socket creation and verification
  fd_socket = socket(AF_INET, SOCK_STREAM, 0);
  if(fd_socket == -1) { printf("tcpserver.c - Socket creation failed.\n"); return(0); }
  else { printf("tcpserver.c - Socket creation successfull.\n"); }

  // Create the server address
  bzero(&addr_server, sizeof(addr_server));
  addr_server.sin_family      = AF_INET;
  addr_server.sin_addr.s_addr = htonl(INADDR_ANY);
  //addr_server.sin_addr.s_addr = inet_addr(IP_SERVER);
  addr_server.sin_port        = htons(PORT_SERVER);

  // Bind the socket to the server address
  int ret = bind(fd_socket, (struct sockaddr *)&addr_server, sizeof(addr_server));
  if (ret != 0) { printf("tcpserver.c - Socket bind failed %s.\n", strerror(errno)); exit(0); }
  else { printf("tcpserver.c - Socket successfully binded.\n"); }

  // Setting options on the server side does not work
  // At least not like this.
  /*
  // Set the options
  bzero((char *)&option_data, sizeof(option_data));
  option_data.type          = 7;
  option_data.length        = 39;
  option_data.pointer       = 4;  // Offset to first address (addrs)

  // Polute the ip packet with additional information which can be used
  // in the shim layer
  for(int i = 3; i < 35; i++)
  {
    option_data.route_data[i] = i + 97;
  }

  ret = setsockopt(fd_socket, IPPROTO_IP, IP_OPTIONS, (char *)&option_data, sizeof(option_data));
  if(ret != 0) { printf("tcpserver.c - Setup of socket options failed: %s.\n", strerror(errno)); exit(0); }
  else { printf("tcpserver.c - Setup of socket options successfull.\n"); }
  */


  ret = listen(fd_socket, 5);
  // Now server is ready to listen and verification
  if (ret != 0) { printf("tcpserver.c - Listen failed.\n"); exit(0); }
  else { printf("tcpserver.c - Server listening.\n"); }

  // Accept the data packet from client and verification
  length = sizeof(addr_client);
  fd_connection = accept(fd_socket, (struct sockaddr *)&addr_client, &length);
  if (fd_connection < 0) { printf("tcpserver.c - Server acccept failed.\n"); exit(0); }
  else { printf("tcpserver.c - Server acccept the client.\n"); }

  // Pass socket to function interacting with the client.
  communicator(fd_connection);

  // Close the socket
  close(fd_socket);

  return(0);
}
