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

 #define IP_SERVER "10.0.2.1"
 #define PORT_SERVER 2727

 #define BUFFER_SIZE 10

void communicator(int fd_socket)
{
  char input_buffer[BUFFER_SIZE];
  char output_buffer[] = "Pong";
  int n;

  while(1)
  {
    bzero(input_buffer, sizeof(input_buffer));
    read(fd_socket, input_buffer, sizeof(input_buffer));
    printf("From Client: %s.\n", input_buffer);
    sleep(2);
    write(fd_socket, output_buffer, sizeof(output_buffer));
  }
}

int main(int argc, char *argv[])
{
  int fd_socket, fd_connection, length;
  struct sockaddr_in addr_server, addr_client;

  // Socket creation and verification
  fd_socket = socket(AF_INET, SOCK_STREAM, 0);
  if(fd_socket == -1) { printf("Socket creation failed.\n"); return(0); }
  else { printf("Socket creation successfull.\n"); }

  // Create the server address
  bzero(&addr_server, sizeof(addr_server));
  addr_server.sin_family      = AF_INET;
  addr_server.sin_addr.s_addr = htonl(INADDR_ANY);
  addr_server.sin_port        = htons(PORT_SERVER);

  // Bind the socket to the server address
  int ret = bind(fd_socket, (struct sockaddr *)&addr_server, sizeof(addr_server));
  if (ret != 0) { printf("Socket bind failed.\n"); exit(0); }
  else { printf("Socket successfully binded.\n"); }

  ret = listen(fd_socket, 5);
  // Now server is ready to listen and verification
  if (ret != 0) { printf("Listen failed.\n"); exit(0); }
  else { printf("Server listening.\n"); }

  // Accept the data packet from client and verification
  length = sizeof(addr_client);
  fd_connection = accept(fd_socket, (struct sockaddr *)&addr_client, &length);
  if (fd_connection < 0) { printf("Server acccept failed.\n"); exit(0); }
  else { printf("Server acccept the client.\n"); }

  // Pass socket to function interacting with the client.
  communicator(fd_connection);

  // Close the socket
  close(fd_socket);

  return(0);
}
