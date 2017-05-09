/*
    Simple udp client
*/
#include<stdio.h> //printf
#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include<arpa/inet.h>
#include<sys/socket.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

#define SERVER "127.0.0.1" // use gethostbyname // getaddrinfo
#define BUFLEN 512  //Max length of buffer
#define PORT 8888   //The port on which to send data

#define MAXDATA 200

#define INITCONNECT 0
#define WAITINITCONNECT 1
#define WAITINGCONFIRMCONNECT 2
#define CONNECTED 3
#define INITCLOSE 4
#define WAITINGCLOSE 5
#define CLOSED 6

typedef struct {

    bool fin;
    bool reset;
    uint64_t seq;
    uint64_t ack;
    uint16_t timeStamp;
    char data;
    uint64_t checkSum;
} Package;


typedef struct {

    Package pack;
    bool ack;

} PackageList;

void die(char *s)
{
    perror(s);
    exit(1);
}

void emptyPackage(Package *packToEmpty)
{
    packToEmpty->fin = false;
    packToEmpty->reset = false;
    packToEmpty->seq = 0;
    packToEmpty->ack = 0;
    packToEmpty->timeStamp = 0;
    packToEmpty->data = '\0';
    packToEmpty->checkSum = 0;

}

uint64_t initSEQ(void)
{

return 1;
}

uint16_t getTimeStamp(void)
{
    // make sure to compare absolute value between current time and timestamp, check if bigger than x minutes/seconds

    // 0-3 min ok, 57-60 ok check values
    return 0;
}

void printPackage(Package pack)
{
    printf("\n fin: %d", pack.fin);
    printf("\n reset: %d", pack.reset);
    printf("\n seq: %u", pack.seq);
    printf("\n ack: %u", pack.ack);
    printf("\n data: %c", pack.data);
    printf("\n checksum: %u\n", pack.checkSum);

}

int main(void)
{


    struct sockaddr_in serverAddr;
    int sock, i, slen=sizeof(serverAddr);
    int currentState = 0;
   // char buf[BUFLEN];
    Package buf;

    //strcpy(buf.data, 'a');
    printf("\n entered data %c \n" , buf.data);
   // char message[BUFLEN] = "test";

    if ( (sock=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }

    memset((char *) &serverAddr, 0, sizeof(serverAddr)); // just sets shit to empty

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);

   // struct sockaddr test;
// use gethostbyname to set sin_addr

    if (inet_aton(SERVER , &serverAddr.sin_addr) == 0)
    {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }

    while(1) {
        switch (currentState) {
            case INITCONNECT:
                emptyPackage(&buf);
                printPackage(buf);
                buf.seq = initSEQ();// creates SEQ from time(NULL)

                if (sendto(sock, &buf, sizeof(Package), 0, (struct sockaddr *) &serverAddr, slen) == -1) {
                    perror("sendto()");
                    exit(1);
                    // die("sendto()"); // fails
                }
                currentState = WAITINITCONNECT;

                break;
            case WAITINITCONNECT:


                break;
            case WAITINGCONFIRMCONNECT:

                break;

            case CONNECTED:

                break;
            case INITCLOSE:

                break;
            case WAITINGCLOSE:

                break;
            case CLOSED:

                break;

        }
    }


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
        printf("Data: %u %u %s\n" , buf.seq, buf.ack, buf.data);
    }

    close(sock);
    return 0;
}