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

 #define IP_SERVER "10.0.0.2"
 #define PORT_SERVER 2727

 struct ip_option_header
 {
   uint8_t type;
   uint8_t length;
   uint8_t pointer;
   uint8_t padding;
   uint8_t route_data[36];
 } option_data;

void communicator(int sockfd)
{

}

int main(int argc, char *argv[])
{
  int fd_socket, fd_connection;
  int ret;
  struct sockaddr_in addr_server, addr_client;

  // Socket creation and verification
  fd_socket = socket(AF_INET, SOCK_STREAM, 0);
  if(fd_socket == -1) { printf("Socket creation failed.\n"); return(0); }
  else { printf("Socket creation successfull.\n"); }

  // Set the options
  bzero((char *)&option_data, sizeof(option_data));
  option_data.type          = 7;
  option_data.length        = 39;
  option_data.pointer       = 4;  // Offset to first address (addrs)

  for(int i = 3; i < 35; i++)
  {
    option_data.route_data[i] = i - 2;
  }

  ret = setsockopt(fd_socket, IPPROTO_IP, IP_OPTIONS, (char *)&option_data, sizeof(option_data));
  if(ret != 0) { printf("Setup of socket options failed: %s.\n", strerror(errno)); exit(0); }
  else { printf("Setup of socket options successfull.\n"); }

  // Create the server address
  bzero(&addr_server, sizeof(addr_server));
  addr_server.sin_family      = AF_INET;
  addr_server.sin_addr.s_addr = inet_addr(IP_SERVER);
  addr_server.sin_port        = htons(PORT_SERVER);

  // Connect the client socket to server socket
  ret = connect(fd_socket, (struct sockaddr *)&addr_server, sizeof(addr_server));
  if (ret != 0) { printf("Connection with the server failed: %s.\n", strerror(errno)); exit(0); }
  else { printf("Connection to the server successfull.\n"); }

  // Pass socket to function interacting with the server.
  communicator(fd_socket);

  // Close the socket
  close(fd_socket);

  return(0);
}
