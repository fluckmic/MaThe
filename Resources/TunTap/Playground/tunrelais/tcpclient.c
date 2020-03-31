/**************************************************************************
 * tcpclient.c                                                            *
 *************************************************************************/

 #include <netdb.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <sys/socket.h>
 #include <arpa/inet.h>
 #include <unistd.h>
 #include <errno.h>

 //#define IP_CLIENT "10.0.1.1"
 //#define PORT_CLIENT 3636
 #define IP_SERVER "10.7.0.9"
 #define PORT_SERVER 2727

 #define BUFFER_SIZE 1000

 struct ip_option_header
 {
   uint8_t type;
   uint8_t length;
   uint8_t pointer;
   uint8_t padding;
   uint8_t route_data[36];
 } option_data;

 void communicator(int fd_socket)
 {
   printf("tcpclient.c - Starting endless data exchange.\n");

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
     sleep(1);
     write(fd_socket, output_buffer, sizeof(output_buffer));
     bzero(input_buffer, sizeof(input_buffer));
     ssize_t nread = read(fd_socket, input_buffer, sizeof(input_buffer));
     if(nread < 1) break;
   }
 }

int main(int argc, char *argv[])
{
  int fd_socket, fd_connection;
  int ret;
  struct sockaddr_in addr_server, addr_client;

  // Socket creation and verification
  fd_socket = socket(AF_INET, SOCK_STREAM, 0);
  if(fd_socket == -1) { printf("tcpclient.c - Socket creation failed.\n"); return(0); }
  else { printf("tcpclient.c - Socket creation successfull.\n"); }

  /*
  // Create the client address for client side binding
  bzero(&addr_client, sizeof(addr_client));
  addr_client.sin_family      = AF_INET;
  addr_client.sin_addr.s_addr = inet_addr(IP_CLIENT);
  addr_client.sin_port        = htons(PORT_CLIENT);

  // Bind the socket to the client address
  ret = bind(fd_socket, (struct sockaddr *)&addr_client, sizeof(addr_client));
  if (ret != 0) { printf("tcpclient.c - Socket bind failed.\n"); exit(0); }
  else { printf("tcpclient.c - Socket successfully binded.\n"); }
  */

  // Set the options
  bzero((char *)&option_data, sizeof(option_data));
  option_data.type          = 7;
  option_data.length        = 39;
  option_data.pointer       = 4;  // Offset to first address (addrs)

  // Polute the ip packet with additional information which can be used
  // in the shim layer
  for(int i = 3; i < 35; i++)
  {
    option_data.route_data[i] = i - 2;
  }

  ret = setsockopt(fd_socket, IPPROTO_IP, IP_OPTIONS, (char *)&option_data, sizeof(option_data));
  if(ret != 0) { printf("tcpclient.c - Setup of socket options failed: %s.\n", strerror(errno)); exit(0); }
  else { printf("tcpclient.c - Setup of socket options successfull.\n"); }

  // Create the server address
  bzero(&addr_server, sizeof(addr_server));
  addr_server.sin_family      = AF_INET;
  addr_server.sin_addr.s_addr = inet_addr(IP_SERVER);
  addr_server.sin_port        = htons(PORT_SERVER);

  // Connect the client socket to server socket
  ret = connect(fd_socket, (struct sockaddr *)&addr_server, sizeof(addr_server));
  if (ret != 0) { printf("tcpclient.c - Connection with the server failed: %s.\n", strerror(errno)); exit(0); }
  else { printf("tcpclient.c - Connection to the server successfull.\n"); }

  // Pass socket to function interacting with the server.
  communicator(fd_socket);

  // Close the socket
  close(fd_socket);

  return(0);
}
