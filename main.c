/*
    Simple udp client
*/
#include<stdio.h> //printf
#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include<arpa/inet.h>
#include<sys/socket.h>
#include <unistd.h>

#define SERVER "127.0.0.1" // use gethostbyname // getaddrinfo
#define BUFLEN 512  //Max length of buffer
#define PORT 8888   //The port on which to send data

#define MAXDATA 200

typedef struct {

    int seq;
    int ack;
    char data[MAXDATA];
} Package;

void die(char *s)
{
    perror(s);
    exit(1);
}

int main(void)
{

    struct sockaddr_in serverAddr;
    int sock, i, slen=sizeof(serverAddr);
   // char buf[BUFLEN];
    Package buf;
    buf.seq = 1;
    buf.ack = 2;

    strcpy(buf.data, "test");
    printf("\n entered data %s \n" , buf.data);
   // char message[BUFLEN] = "test";

    if ( (sock=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }

    memset((char *) &serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);

    struct sockaddr test;


    if (inet_aton(SERVER , &serverAddr.sin_addr) == 0)
    {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }

    // makeSeq()

    // SYN!




    // ELSE QUIT??



    while(1)
    {

        //send the message
        if (sendto(sock, &buf, sizeof(buf) , 0 , (struct sockaddr *) &serverAddr, slen)==-1)
        {
            die("sendto()"); // fails
        }

        //receive a reply and print it
        //clear the buffer by filling null, it might have previously received data
        memset(&buf,'\0', BUFLEN);
        //try to receive some data, this is a blocking call
        if (recvfrom(sock, &buf, BUFLEN, 0, (struct sockaddr *) &serverAddr, &slen) == -1)
        {
            die("recvfrom()"); //fails
        }

       // puts(buf);
        printf("Data: %d %d %s\n" , buf.seq, buf.ack, buf.data);
    }

    close(sock);
    return 0;
}