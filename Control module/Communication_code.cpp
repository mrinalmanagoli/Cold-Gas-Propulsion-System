/*
Author: Mrinal Managoli
This is code that needs to be integrated with the cotrol module at the very end.
This is essential for establishing communications between the control and communication modules.
*/

#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#define WIN32_LEAN_AND_MEAN
#include <string.h>

#define DEFAULT_PORT 5555
#define ADDRESS "localhost"
#define DEFAULT_PROTO SOCK_STREAM
#define NUMBER_OF_BYTES_TO_SEND 4

SOCKET conn_socket;
struct sockaddr_in server;
char const ready = 0xFE;

/*
Purpose: 
    To initialize all socket parameters and establish a connection with the server
Return: 
    0 if a successful connection is established
    1 if there's a failure. In the event of a failure, an appropriate error message is printed
*/
int init_connection();

/*
Purpose: 
    To send a float to the ground station communications sub-module as a single-precision value.
    Serialized as 4 bytes and sent
Return:
    0 if the data couldn't be sent
    1 if the data was sent successfully
*/
int send_data(float value);

/*
Purpose:
    To terminate connection with the server
*/
void terminate_connection();

/*
Purpose:
    To receive data from the server over the socket
Parameters:
    char* buffer: The buffer in which data that is read will be stored
    length: Size of the buffer
Return: Number of bytes read
*/
int receive_data(char* buffer, int length);

int main(int argc, char **argv)
{
    if (!init_connection()) 
    {   //First, establish a connection with server on port 5555

        //Next, issue a blocking receive call to know when comms. is ready to go
        char recvbuf[1];
        int receive_bytes = receive_data(recvbuf, 1);

        if (recvbuf[0] == ready)
        {   //Comms. module signals all is good to go, proceed...

            //printf("Received ready signal from ground station communications sub-module!\n");

            //TODO: Nic's controller code in here
            
            //This is just test code to send to the ground station comms. sub-module,
            send_data((float)-12.78);
            send_data((float)14.67);
        }
    }
    else
    {   //Something went wrong with the socket connection initialization.
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;  
}

int init_connection()
{
    WSADATA wsaData;
    if (WSAStartup(0x202, &wsaData) != 0)
    {

        printf("Failed to initialize client in init_socket(). Exiting ...\n");
        WSACleanup();
        return 1;
    }
    else
    {
        printf("Socket initialized!\n");

        if (DEFAULT_PORT == 0)
        {
            printf("Invalid port specified! Exiting ...\n");
            return 1;
        }
        else
        {
            printf("Port validated!\n");
            
            struct hostent *hp;
            char const *server_name= ADDRESS;
            unsigned int addr;

            if (isalpha(server_name[0]))
            {   // server address is a name
                hp = gethostbyname(server_name);
            }
            else
            { 
                addr = inet_addr(server_name);
                hp = gethostbyaddr((char *)&addr, 4, AF_INET);
            }

            if (hp == NULL )
            {
                printf("Client: Cannot resolve address \"%s\": Error %d\n", server_name, WSAGetLastError());
                WSACleanup();
                return 1;
            }
            else
            {
                printf("Client initialization is OK.\n");

                memset(&server, 0, sizeof(server));
                memcpy(&(server.sin_addr), hp->h_addr, hp->h_length);
                server.sin_family = hp->h_addrtype;
                server.sin_port = htons(DEFAULT_PORT);

                conn_socket = socket(AF_INET, DEFAULT_PROTO, 0); 

                if (conn_socket < 0 )
                {
                    printf("Error Opening socket: Error %d\n", WSAGetLastError());
                    WSACleanup();
                    return 1;
                }
                else
                {
                    printf("Client: socket() is OK.\n");

                    if (connect(conn_socket, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
                    {
                        printf("Client: connect() failed: %d\n", WSAGetLastError());
                        WSACleanup();
                        return 1;
                    }
                    else
                    {
                        printf("Client: connect() is OK.\n");
                        return 0;
                    }
                }    
            }
        }
    }
}

int send_data(float value)
{
    union {
        float a;
        unsigned char bytes[NUMBER_OF_BYTES_TO_SEND];
    } send_data;

    send_data.a = value;
    int ret_val = 1;

    for (int ii = NUMBER_OF_BYTES_TO_SEND - 1; ii >= 0 ; ii--)
    {
        //printf("%u\n", send_data.bytes[ii]);
        char data[] = {(char)send_data.bytes[ii]};
        ret_val &= send(conn_socket, data, sizeof(data), 0);
    }
    return ret_val != SOCKET_ERROR && ret_val != 0;
}

int receive_data(char* buffer, int length)
{
    return recv(conn_socket, buffer, length, 0);
}

void terminate_connection()
{
    closesocket(conn_socket);
    WSACleanup();
}