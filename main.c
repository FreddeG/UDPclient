/*
    Simple udp client
*/

#include "list.h"
#include "Generic.h"

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
#define WINDOWSIZE 3

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

int main(void)
{


    struct sockaddr_in serverAddr;
    int sock, i, slen=sizeof(serverAddr);
    uint8_t count = 0;
    int currentState = 0;
    // char buf[BUFLEN];
    char charFromFile;
    Package outputBuf, inputBuf;
    FILE *fp = NULL;
    uint16_t freeWin = WINDOWSIZE;
    bool endFlag = false;

    List list;
    list.head = NULL;

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
                        // Gör något;
                    }


                    // break
                }
                else
                {
                    // if timeout, resend
                    if(count<3)
                    {
                        count++;

                        printf("\n timeout occured!");
                        if (sendto(sock, &outputBuf, sizeof(Package), 0, (struct sockaddr *) &serverAddr, slen) == -1)
                        {
                            die("sendto()");
                        }
                        printf("\n Output package");
                        printPackage(outputBuf);
                    }
                    else
                    {
                        uint contFlag = 0;
                        contFlag = yesNoRepeater("No responce from the server! Continue?");
                        if(contFlag)
                        {
                            currentState = INITCONNECT;
                        }
                        else
                        {
                            exit(EXIT_FAILURE);
                        }
                    }

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
                    printf("\n REACHED CONNECTED!");
                    if(fp == NULL)
                    {
                        fp = fopen ("file.txt", "r");
                    }
                    else
                    {
                        return 0; // solve loop somehow to exit when EOF is found
                    }
                    if(fp == NULL) // error
                    {
                        die("fopen");
                    }
                }

                break;

            case CONNECTED:

                //printf("\n Type something to add to list");
                //printPackage(outputBuf);
                //scanf(" %c", &outputBuf.data);

                //Send packages until window full. Check that we have not reached the last package.
                if(freeWin != 0 && endFlag == false)
                {
                    charFromFile = fgetc(fp);
                    if (charFromFile != EOF)
                    {
                        outputBuf.data = charFromFile;
                        outputBuf.checkSum = 0;
                        outputBuf.checkSum = checksum(outputBuf); // remember to set checksum to 0 before in normal cases
                        addNodeLast(&list, outputBuf);
                        if (sendto(sock, &outputBuf, sizeof(Package), 0, (struct sockaddr*) &serverAddr, slen) == -1)
                        {
                            die("sendto()");
                        }
                        printf("\n Output package");
                        freeWin--;
                    }
                    else //We have reached the last package. Time to prepare shutdown.
                    {
                        endFlag = true;
                    }

                }
                else if(list.head != NULL) //We have sent all of the packages in the window and are w8ing for ack:s.
                {
                    timeout_t.tv_sec = 5; // Time that we listen for acks b4 resending the window.
                    timeout_t.tv_usec = 0;
                    if (select(FD_SETSIZE, &readFdSet, NULL, NULL, &timeout_t) < 0) { // blocking, waits until one the FD is set to ready, will keep the ready ones in readFdSet
                        perror("Select failed\n");
                        exit(EXIT_FAILURE);
                    }
                    else if(FD_ISSET(sock, &readFdSet)) // if true we got a package so read it.
                    {
                        if ((recvfrom(sock, &inputBuf, sizeof(Package), 0, (struct sockaddr *) &serverAddr, &slen)) ==
                            -1) {
                            // perror(recvfrom());
                            die("recvfrom()");
                        }

                        if(viewPackage(inputBuf) == 3)
                        {
                            // Oldpack! Discard!
                            if(list.head->data.seq < inputBuf.ack)
                            {
                                if(list.head->data.seq + WINDOWSIZE <= inputBuf.ack)// Should not happen!
                                {
                                    printf("\nDebug: List exeeded");
                                    getchar();
                                    exit(1);
                                }

                                while(list.head->data.seq <= inputBuf.ack)
                                {
                                    removeFirst(&list);
                                    if (endFlag == false)
                                    {
                                        freeWin++;
                                    }
                                    if(list.head == NULL)
                                    {
                                        break;
                                    }
                                }
                            }
                            else
                            {
                                //Do nothing
                            }

                        }
                    }
                    else
                    {
                        //Resends the Current window!
                        Node *current = list.head;
                        Node *previous = current;
                        while (current != NULL)
                        {
                            previous = current;
                            outputBuf = previous->data;
                            current = current->Next;
                            if (sendto(sock, &outputBuf, sizeof(Package), 0, (struct sockaddr*) &serverAddr, slen) == -1)
                            {
                                die("sendto()");
                            }
                        }

                    }
                }
                else
                {
                    currentState = INITCLOSE;
                    printf("\n");
                }



            printf("\nNumber of nodes: %d", numberOfNodes(&list));
            printList(&list);
            getchar();
            // WE ARE DONE !!!
            // currentState = INITCLOSE;


                break;

            case INITCLOSE:



                break;
            case WAITINGCLOSE:

                break;
            case CLOSED:

            break;
        }

    }
    return 0;
}

/*
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

}*/