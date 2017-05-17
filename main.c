/*
    Simple udp client. DOES NOT CHECK FOR OTHER INCOMING CONNECTIONS YET!
*/

#include "list.h"


#define SERVER "127.0.0.1" // use gethostbyname // getaddrinfo
#define PORT 8888   //The port on which to send data
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
    int sock, slen = sizeof(serverAddr);
    uint8_t count = 0;
    int currentState = 0;
    // char buf[BUFLEN];
    char charFromFile;
    Package outputBuf, inputBuf;
    FILE *fp = NULL;
    uint16_t freeWin = WINDOWSIZE;
    bool endFlag = false;
    uint64_t incLowAck = 0;

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
            {
                emptyPackage(&outputBuf);
                incLowAck = initSEQ(); // creates SEQ from time(NULL)
                outputBuf.seq = incLowAck;
                outputBuf.syn = true;
                outputBuf.checkSum = checksum(outputBuf);

                if (sendto(sock, &outputBuf, sizeof(Package), 0, (struct sockaddr *) &serverAddr, slen) == -1) {
                    perror("sendto()");
                    exit(1);
                    // die("sendto()"); // fails
                }

                // Errortracing code
                printf("\n Output package");
                printPackage(outputBuf);
                printf("\nincLowAck:%zu ", incLowAck);

                currentState = WAITINITCONNECT;

                break;
            }


            case WAITINITCONNECT:
            {
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
                    //If recfrom fails (returns -1) then terminate.
                    if ((recvfrom(sock, &inputBuf, sizeof(Package), 0, (struct sockaddr *) &serverAddr, &slen)) == -1) {
                        // perror(recvfrom());
                        die("recvfrom()");
                    }

                    // Errortracing code
                    printf("\n Input package");
                    printPackage(inputBuf);
                    printf("\nincLowAck:%zu " ,incLowAck);

                    // If the pacage is of right type and has the expected ack move on!
                    if((viewPackage(inputBuf) == 2) && (inputBuf.ack == incLowAck)) // 2
                    {
                        emptyPackage(&outputBuf);
                        incLowAck = inputBuf.ack + 1;
                        outputBuf.seq = incLowAck;
                        outputBuf.ack = inputBuf.seq;
                        checksum(outputBuf);

                        if (sendto(sock, &outputBuf, sizeof(Package), 0, (struct sockaddr*) &serverAddr, slen) == -1)
                        {
                            die("sendto()");
                        }

                        // Errortracing code
                        printf("\n Output package");
                        printPackage(outputBuf);
                        printf("\nincLowAck:%zu ", incLowAck);

                        currentState = WAITINGCONFIRMCONNECT;
                    }
                    else
                    {
                        printf("\n Unexpected or broken package!\n");
                    }
                }
                else
                {
                    // if timeout, resend up to 3 times
                    if(count<3)
                    {
                        count++;

                        printf("\n timeout occured!");
                        if (sendto(sock, &outputBuf, sizeof(Package), 0, (struct sockaddr *) &serverAddr, slen) == -1)
                        {
                            die("sendto()");
                        }

                        // Errortracing code
                        printf("\n Output package");
                        printPackage(outputBuf);
                        printf("\nincLowAck:%zu ", incLowAck);
                    }
                    //Host unresponsive during the establish connection phase. Abort?
                    else
                    {
                        uint contFlag = 0;
                        contFlag = yesNoRepeater("No responce from the server! Continue?");
                        if(contFlag)
                        {
                            //Try again!
                            currentState = INITCONNECT;
                        }
                        else
                        {
                            //Terminate!
                            exit(EXIT_FAILURE);
                        }
                    }
                }

                break;
            }


            case WAITINGCONFIRMCONNECT:
            {
                readFdSet = activeFdSet;
                timeout_t.tv_sec = 5; // needs bigger timeout than server
                timeout_t.tv_usec = 0;

                // Calls select with a time counter. if we dont "recieve" during this time. Assume connection success.
                if (select(FD_SETSIZE, &readFdSet, NULL, NULL, &timeout_t) < 0) { // blocking, waits until one the FD is set to ready, will keep the ready ones in readFdSet
                    perror("Select failed\n");
                    exit(EXIT_FAILURE);
                }
                else if(FD_ISSET(sock, &readFdSet))
                {
                    // Something has come in, recieve it!
                    if ((recvfrom(sock, &inputBuf, sizeof(Package), 0, (struct sockaddr *) &serverAddr, &slen)) == -1) {
                        // perror(recvfrom());
                        die("recvfrom()");
                    }

                    // Errortracing code
                    printf("\n Input package");
                    printPackage(inputBuf);
                    printf("\nincLowAck:%zu ", incLowAck);


                    // If we get a resent syn+ack, resend the ack.
                    if((viewPackage(inputBuf)) == 2 && ((incLowAck-1) == inputBuf.ack)) // 2
                    {
                        //if sendto fails (return -1) terminate
                        if (sendto(sock, &outputBuf, sizeof(Package), 0, (struct sockaddr*) &serverAddr, slen) == -1)
                        {
                            die("sendto()");
                        }

                        // Errortracing code
                        printf("\n Output package");
                        printPackage(outputBuf);
                        printf("\nincLowAck:%zu ", incLowAck);


                    }
                    else
                    {
                        printf("\nUnexpected or broken package\n");
                        /* weird package/broken package, causes timeout reset,
                         * meaning a lot of broken packages mean we never establish a connection
                         */
                    }


                    // break
                }
                else // SUCCSESS! Open the filepointer we want to transmit.
                {
                    currentState = CONNECTED;
                    printf("\n REACHED CONNECTED!");
                    if(fp == NULL)
                    {
                        fp = fopen ("file.txt", "r");
                    }
                    else
                    {
                        exit(EXIT_FAILURE); // solve loop somehow to exit when EOF is found
                    }
                    if(fp == NULL) // error
                    {
                        die("fopen");
                    }
                }
                break;
            }


            case CONNECTED:
            {
                //Send packages until window full. Check that we have not reached the last package.
                if((freeWin != 0) && (endFlag == false))
                {
                    charFromFile = fgetc(fp);
                    if (charFromFile != EOF)
                    {
                        //If this is the start of the transmission, set SEQ with lowIncAck (from handshake).
                        if((list.head == NULL))
                        {
                            outputBuf.seq = incLowAck + 1;
                        }
                        //This is not the first time we send data: continue from last seq.
                        else
                        {
                            outputBuf.seq = outputBuf.seq + 1;
                        }

                        outputBuf.ack = inputBuf.seq;
                        outputBuf.data = charFromFile;
                        outputBuf.checkSum = 0;
                        outputBuf.checkSum = checksum(outputBuf);

                        //Put the item in the "window".
                        addNodeLast(&list, outputBuf);

                        //If send fails (returns -1) terminate else move on.
                        if (sendto(sock, &outputBuf, sizeof(Package), 0, (struct sockaddr*) &serverAddr, slen) == -1)
                        {
                            die("sendto()");
                        }

                        // Errortracing code
                        printf("\n Output package");
                        printPackage(outputBuf);
                        printf("\nincLowAck:%zu ", incLowAck);
                        freeWin--;
                    }
                    else //We have reached the last package. Time to prepare shutdown.
                    {
                        endFlag = true;
                    }
                }
                else if(list.head != NULL) //We have sent all of the packages in the window and still w8ing for ack:s.
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

                        //If the package contains data!
                        if(viewPackage(inputBuf) == 3)
                        {
                            // Oldpack! Discard!
                            if(list.head->data.seq < inputBuf.ack)
                            {
                                if(list.head->data.seq + WINDOWSIZE-1 <= inputBuf.ack)// Should not happen!
                                {
                                    printf("\nDebug: Ack exeede sent seq!");
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
                        while (current != NULL)
                        {
                            outputBuf = current->data;
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
                break;
            }

            case INITCLOSE:
            {
                break;
            }
            case WAITINGCLOSE:
            {
                break;
            }
            case CLOSED:
            {
                break;
            }
            default:
            {
                break;
            }
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