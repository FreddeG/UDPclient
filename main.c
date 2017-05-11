/*
    Simple udp client
*/

#include "list.h"

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

/*
typedef struct {

    bool fin;
    bool reset;
    bool syn;
    uint64_t seq;
    uint64_t ack;
    uint16_t timeStamp;
    char data;
    uint64_t checkSum;
} Package;
*/

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
    packToEmpty->syn = false;
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

uint8_t viewPackage(Package pack)
{

    if(1) // checksum
    {
        if(pack.syn == true) return 1; // syn

        if(pack.fin == true) return 4; // last package

        if(pack.reset == true) return 5; // server resets connection

        if(pack.data == '\0') return 2; // ack + seq, no data

        if(pack.data != '\0') return 3; // ack + seq with data, package to read

        printf("\n ERROR!");
        exit(1);

    }

    //checksum wrong return
    // 0 bad case, (checksum fail)
    // 1 syn (seq, no data, no ack)
    // 2 ack (seq + ack, no data)
    // 3 data (seq + ack + data)
    // 4 fin (fin = true, rest doesn't matter)
    // 5 reset

}

void printPackage(Package pack)
{
    printf("\n fin: %d", pack.fin);
    printf("\n reset: %d", pack.reset);
    printf("\n syn: %d", pack.syn);
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
    Package outputBuf, inputBuf;

    fd_set activeFdSet, readFdSet;
    struct timeval timeout_t;

    //strcpy(buf.data, 'a');
    //printf("\n entered data %c \n" , outputBuf.data);
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
    /* Initialize the set of active sockets */
    FD_ZERO(&activeFdSet); // initializes an empty FDset
    FD_SET(sock, &activeFdSet); // puts socket in the set, activeFDset currently only contains this socket.

    while(1) {
        switch (currentState) {
            case INITCONNECT:
                emptyPackage(&outputBuf);
                outputBuf.seq = initSEQ();// creates SEQ from time(NULL)
                outputBuf.syn = true;

                if (sendto(sock, &outputBuf, sizeof(Package), 0, (struct sockaddr *) &serverAddr, slen) == -1) {
                    perror("sendto()");
                    exit(1);
                    // die("sendto()"); // fails
                }
                printf("\n Output package");
                printPackage(outputBuf);

                currentState = WAITINITCONNECT;

                break;
            case WAITINITCONNECT:



                readFdSet = activeFdSet;
                // lägg in time envl
                timeout_t.tv_sec = 10;
                timeout_t.tv_usec = 0;


                if (select(FD_SETSIZE, &readFdSet, NULL, NULL, &timeout_t) < 0) { // blocking, waits until one the FD is set to ready, will keep the ready ones in readFdSet
                    perror("Select failed\n");
                    exit(EXIT_FAILURE);
                }
                else if(FD_ISSET(sock, &readFdSet))
                {

                    if ((recvfrom(sock, &inputBuf, sizeof(Package), 0, (struct sockaddr *) &serverAddr, &slen)) == -1) {
                        // perror(recvfrom());
                        die("recvfrom()");
                    }
                    printf("\n Input package");
                    printPackage(inputBuf);

                    if(viewPackage(inputBuf) == 2) // 2
                    {
                        emptyPackage(&outputBuf);
                        outputBuf.seq = inputBuf.ack;
                        outputBuf.ack = inputBuf.seq +1;

                        if (sendto(sock, &outputBuf, sizeof(Package), 0, (struct sockaddr*) &serverAddr, slen) == -1)
                        {
                            die("sendto()");
                        }
                        printf("\n Output package");
                        printPackage(outputBuf);

                        currentState = WAITINGCONFIRMCONNECT;

                    }
                    else
                    {
                        //
                    }


                    // break
                }
                else
                {
                    // if timeout, resend
                    printf("\n timeout occured!");
                    if (sendto(sock, &outputBuf, sizeof(Package), 0, (struct sockaddr*) &serverAddr, slen) == -1)
                    {
                        die("sendto()");
                    }
                    printf("\n Output package");
                    printPackage(outputBuf);
                    // want to quit?

                    break;

                }


                break;
            case WAITINGCONFIRMCONNECT:

                readFdSet = activeFdSet;
                // lägg in time envl
                timeout_t.tv_sec = 5; // needs bigger timeout than server
                timeout_t.tv_usec = 0;


                if (select(FD_SETSIZE, &readFdSet, NULL, NULL, &timeout_t) < 0) { // blocking, waits until one the FD is set to ready, will keep the ready ones in readFdSet
                    perror("Select failed\n");
                    exit(EXIT_FAILURE);
                }
                else if(FD_ISSET(sock, &readFdSet))
                {

                    if ((recvfrom(sock, &inputBuf, sizeof(Package), 0, (struct sockaddr *) &serverAddr, &slen)) == -1) {
                        // perror(recvfrom());
                        die("recvfrom()");
                    }
                    printf("\n Input package");
                    printPackage(inputBuf);
                    // causes problems if wrong client tries to connect

                    if(viewPackage(inputBuf) == 2) // 2
                    {


                        if (sendto(sock, &outputBuf, sizeof(Package), 0, (struct sockaddr*) &serverAddr, slen) == -1)
                        {
                            die("sendto()");
                        }
                        printf("\n Output package");
                        printPackage(outputBuf);


                    }
                    else
                    {
                        // weird package/broken package, causes timeout reset, meaning a lot of broken packages mean we never establish a connection
                    }


                    // break
                }
                else
                {
                    currentState = CONNECTED;

                }

                break;

            case CONNECTED:
                printf("\n REACHED CONNECTED!");
                getchar();
                currentState = INITCLOSE;



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
        if (sendto(sock, &outputBuf, sizeof(outputBuf) , 0 , (struct sockaddr *) &serverAddr, slen)==-1)
        {
            die("sendto()"); // fails
        }

        //receive a reply and print it
        //clear the buffer by filling null, it might have previously received data
        memset(&outputBuf,'\0', BUFLEN);
        //try to receive some data, this is a blocking call
        if (recvfrom(sock, &outputBuf, BUFLEN, 0, (struct sockaddr *) &serverAddr, &slen) == -1)
        {
            die("recvfrom()"); //fails
        }

       // puts(buf);
        printf("Data: %u %u %s\n" , outputBuf.seq, outputBuf.ack, outputBuf.data);
    }

    close(sock);
    return 0;
}