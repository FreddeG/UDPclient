/*
    Simple udp client. DOES NOT CHECK FOR OTHER INCOMING CONNECTIONS YET!
*/

#include "list.h"
#include "Generic.h"




int main(void)
{

    //UDP Stuff
    struct sockaddr_in serverAddr;
    int sock;
    int currentState = 0;
    char charFromFile;
    socklen_t slen = sizeof(serverAddr);
    fd_set activeFdSet, readFdSet;
    struct timeval timeout_t;

    //Variables for package and List!
    Package outputBuf, inputBuf;
    List list, jailList;
    jailList.head = NULL;
    list.head = NULL;

    //Filepointer for packet data
    FILE *fp = NULL;

    //Variables for sliding window and error prob.
    uint8_t count = 0;
    uint16_t freeWin = 0;
    uint16_t winSize = 0;
    uint16_t trigger = 0;
    uint16_t packType = 0;
    uint64_t incMaxAck = 0;
    bool endFlag = false;
    bool genError = true;

    //Initiate random Seed
    srand(time(0));

    //Initiate socket.
    if ( (sock=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }
    memset((char *) &serverAddr, 0, sizeof(serverAddr)); // just sets shit to empty
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);


    if (inet_aton(SERVER , &serverAddr.sin_addr) == 0)
    {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }
    /* Initialize the set of active sockets */
    FD_ZERO(&activeFdSet); // initializes an empty FDset
    FD_SET(sock, &activeFdSet); // puts socket in the set, activeFDset currently only contains this socket.

    emptyPackage(&outputBuf);

    while(1) {
        switch (currentState) {
            case INITCONNECT:
            {
                //Create the package!
                emptyPackage(&outputBuf);
                incMaxAck = initSEQ(); // creates SEQ from time(NULL)
                outputBuf.seq = incMaxAck;
                outputBuf.syn = true;
                outputBuf.checkSum = checksum(outputBuf);

                //Send it? Jail might generate an error if flag errorGen is set to True
                jail(&jailList, outputBuf, sock, serverAddr, genError);

                // Errortracing code
                printf("\n-=First SYN=-");
                printPackage(outputBuf);
                printf("incLowAck:%zu \n", incMaxAck);

                //Change State!
                currentState = WAITINITCONNECT;

                //Release from jail? With some probablility, release a random package from the jailList!
                trigger = rand() %100;
                if(trigger > RELEASEPROBABILITY)
                {
                    Node *ptr = freeFromJail(&jailList);
                    if(ptr != NULL)
                    {
                        if (sendto(sock, &ptr->data, sizeof(Package), 0, (struct sockaddr *) &serverAddr, slen) == -1) {
                            perror("sendto()");
                            exit(1);
                            // die("sendto()"); // fails
                        }
                        printf("\n-=JAIL RELEASE=-");
                        printPackage(ptr->data);
                        free(ptr);
                        ptr = NULL;
                    }
                }
                break;
            } //End of INITCONNECT LOOP


            case WAITINITCONNECT:
            {
                //Initiate the Sets!
                readFdSet = activeFdSet;

                //Set Timeout values for this set
                timeout_t.tv_sec = 10;
                timeout_t.tv_usec = 0;

                //Listen for incoming packages!
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
                    printf("\n-=Input package=-");
                    printPackage(inputBuf);
                    packType = viewPackage(inputBuf);

                    // If the package is of right type and has the expected ack CHANGE STATE!
                    if((packType == 2) && (inputBuf.ack == incMaxAck)) // 2
                    {
                        //Error checking code, window cant be 2 small.
                        if((winSize = inputBuf.winSize) <=1)
                        {
                            exit(EXIT_FAILURE);
                        }

                        //Initiate Window!
                        freeWin = winSize;

                        //Create package!
                        emptyPackage(&outputBuf);
                        incMaxAck = inputBuf.ack + 1;
                        outputBuf.seq = incMaxAck;
                        outputBuf.ack = inputBuf.seq;
                        outputBuf.checkSum = checksum(outputBuf);

                        //Send it? Jail might generate an error if flag errorGen is set to True
                        jail(&jailList, outputBuf, sock, serverAddr, genError);

                        // Errortracing code
                        printf("incLowAck:%zu \n", incMaxAck);

                        //Change state and reset count!
                        count = 0;
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
                    if(count < 3)
                    {
                        count++;
                        printf("\n timeout occured!");

                        //Send it? Jail might generate an error if flag errorGen is set to True
                        jail(&jailList, outputBuf, sock, serverAddr, genError);

                        // Errortracing code
                        printf("incLowAck:%zu \n", incMaxAck);
                    }
                    //Host unresponsive during the establish connection phase. Abort or restart in this state?
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

                //Release from jail? With some probablility, release a random package from the jailList!
                trigger = rand() %100;
                if(trigger > RELEASEPROBABILITY)// jail release probablity
                {
                    Node *ptr = freeFromJail(&jailList);
                    if(ptr != NULL)
                    {
                        if (sendto(sock, &ptr->data, sizeof(Package), 0, (struct sockaddr *) &serverAddr, slen) == -1) {
                            perror("sendto()");
                            exit(1);
                            // die("sendto()"); // fails
                        }
                        printf("\n-=JAIL RELEASE=-");
                        printPackage(ptr->data);
                        free(ptr);
                        ptr = NULL;
                    }
                }
                break;
            }//End of WAITINITCONNECT LOOP


            case WAITINGCONFIRMCONNECT:
            {
                readFdSet = activeFdSet;
                timeout_t.tv_sec = 20; // needs bigger timeout than server
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
                    printf("\n-=Input package=-");
                    printPackage(inputBuf);

                    // If we get a resent syn+ack, resend the ack.
                    packType = viewPackage(inputBuf);
                    if(packType == 2 && ((incMaxAck-1) == inputBuf.ack)) // 2
                    {
                        //Send it? Jail might generate an error if flag errorGen is set to True
                        jail(&jailList, outputBuf, sock, serverAddr, genError);

                        // Errortracing code
                        printf("incLowAck:%zu \n", incMaxAck);
                    }
                    else
                    {
                        printf("\nUnexpected or broken package\n");
                        /* weird package/broken package, causes timeout reset,
                         * meaning a lot of broken packages mean we never establish a connection
                         */
                    }
                }
                else // SUCCSESS! Open the filepointer we want to transmit.
                {
                    count = 0;

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



                trigger = rand() %100;


                if(trigger > RELEASEPROBABILITY)// jail release probablity
                {
                    Node *ptr = freeFromJail(&jailList);
                    if(ptr != NULL)
                    {
                        if (sendto(sock, &ptr->data, sizeof(Package), 0, (struct sockaddr *) &serverAddr, slen) == -1) {
                            perror("sendto()");
                            exit(1);
                            // die("sendto()"); // fails
                        }
                        printf("\n-=JAIL RELEASE=-");
                        printPackage(ptr->data);
                        free(ptr);
                        ptr = NULL;
                    }
                }

                break;
            }


            case CONNECTED:
            {
                readFdSet = activeFdSet;
                printf("\nfreeWin: %hu, endFlag: %d\n\n", freeWin, endFlag);
                //Send packages until window full. Check that we have not reached the last package.
                if((freeWin != 0) && (endFlag == false))
                {
                    charFromFile = fgetc(fp);
                    if (charFromFile != EOF)
                    {
                        //If this is the start of the transmission, set SEQ with lowIncAck (from handshake).
                        if((list.head == NULL))
                        {
                            incMaxAck++;
                            outputBuf.seq = incMaxAck;
                        }
                        //This is not the first time we send data: continue from last seq.
                        else
                        {
                            Node* last = retrieveLastNode(list);
                            outputBuf.seq = last->data.seq+1;
                            incMaxAck++;
                        }

                        outputBuf.ack = inputBuf.seq;
                        outputBuf.data = charFromFile;
                        outputBuf.checkSum = 0;
                        outputBuf.checkSum = checksum(outputBuf);

                        //Put the item in the "window".
                        addNodeLast(&list, outputBuf);

                        jail(&jailList, outputBuf, sock, serverAddr, genError);
                        /*
                        //If send fails (returns -1) terminate else move on.
                        if (sendto(sock, &outputBuf, sizeof(Package), 0, (struct sockaddr*) &serverAddr, slen) == -1)
                        {
                            die("sendto()");
                        }
*/
                        // Errortracing code
                        printf("incLowAck:%zu \n", incMaxAck);
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

                        // Errortracing Code!
                        printf("\n-=Input package=-");
                        printPackage(inputBuf);

                        packType = viewPackage(inputBuf);
                        //If the package contains ack+seq!
                        if(packType== 2)
                        {
                            // Oldpack! Discard!
                            if(list.head->data.seq <= inputBuf.ack)
                            {
                                if(list.head->data.seq + winSize -1 < inputBuf.ack)// Should not happen!
                                {
                                    printf("\nDebug: Ack exceed sent seq!");
                                    getchar();
                                    exit(1);
                                }

                                while(list.head->data.seq <= inputBuf.ack)
                                {
                                    removeFirst(&list);
                                    if (endFlag == false)
                                    {
                                        printf("\nMOVING WIN!");
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
                                printf("\nOldpack! Discard!\n");
                            }
                        }
                        else
                        {
                            printf("\nWrong Type or Broken package!\n");
                        }
                    }
                    else
                    {
                        if (count < 4)
                        {
                            count++;
                            //Resends the Current window!
                            Node *current = list.head;
                            while (current != NULL)
                            {
                                outputBuf = current->data;
                                current = current->Next;

                                jail(&jailList, outputBuf, sock, serverAddr, genError);
                                /*
                                if (sendto(sock, &outputBuf, sizeof(Package), 0, (struct sockaddr*) &serverAddr, slen) == -1)
                                {
                                    die("sendto()");
                                }*/
                                printf("\nResending Window!\n-=Output package=-");
                                printPackage(outputBuf);
                                printf("incLowAck:%zu \n", incMaxAck);

                            }
                        }
                        else
                        {
                            uint contFlag = 0;
                            contFlag = yesNoRepeater("No responce from Server! retry?");
                            if(contFlag)
                            {
                                //Try again!
                                currentState = CONNECTED;
                            }
                            else
                            {
                                //Terminate!
                                exit(EXIT_FAILURE);
                            }
                        }
                    }
                }
                else
                {
                    count = 0;
                    emptyPackage(&outputBuf);
                    outputBuf.fin = true;
                    incMaxAck++;
                    outputBuf.seq = incMaxAck;
                    outputBuf.ack = inputBuf.seq;
                    outputBuf.checkSum = checksum(outputBuf);

                    jail(&jailList, outputBuf, sock, serverAddr, genError);
                    /*
                    //If send fails (returns -1) terminate else move on.
                    if (sendto(sock, &outputBuf, sizeof(Package), 0, (struct sockaddr*) &serverAddr, slen) == -1)
                    {
                        die("sendto()");
                    }
*/
                    currentState = INITCLOSE;

                    //Errortracing Code!
                    printf("-=SENDING FIN!=-");
                    printPackage(outputBuf);
                    printf("incLowAck:%zu \n", incMaxAck);
                    printf("\n\nReached INITCLOSED!");
                }

                trigger = rand() %100;
                if(trigger > RELEASEPROBABILITY)// jail release probablity
                {
                    Node *ptr = freeFromJail(&jailList);
                    if(ptr != NULL)
                    {
                        if (sendto(sock, &ptr->data, sizeof(Package), 0, (struct sockaddr *) &serverAddr, slen) == -1) {
                            perror("sendto()");
                            exit(1);
                            // die("sendto()"); // fails
                        }
                        printf("\n-=JAIL RELEASE=-");
                        printPackage(ptr->data);
                        free(ptr);
                        ptr = NULL;
                    }
                }


                break;
            }

            case INITCLOSE:
            {
                // Listen for fin+ack. Allow Resend of FIN!
                readFdSet = activeFdSet;
                timeout_t.tv_sec = 20; // Time that we listen for acks b4 resending the window.
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

                    // Errortracing Code!
                    printf("\n-=Input package=-");
                    printPackage(inputBuf);

                    packType = viewPackage(inputBuf);
                    //If the package contains data!
                    if(packType == 4 && inputBuf.ack == incMaxAck)
                    {
                        emptyPackage(&outputBuf);
                        outputBuf.fin = true;
                        incMaxAck++;
                        outputBuf.seq = incMaxAck;
                        outputBuf.ack = inputBuf.seq;
                        outputBuf.checkSum = checksum(outputBuf);

                        jail(&jailList, outputBuf, sock, serverAddr, genError);
                        /*
                        //If send fails (returns -1) terminate else move on.
                        if (sendto(sock, &outputBuf, sizeof(Package), 0, (struct sockaddr*) &serverAddr, slen) == -1)
                        {
                            die("sendto()");
                        }*/

                        printf("-=LAST ACK!=-");
                        printPackage(outputBuf);

                        currentState = WAITINGCLOSE;
                    }
                    else
                    {
                        printf("\nDID NOT RECIEVE EXPECTED FinACK!");
                    }
                }
                else
                {
                    if(count < 4)
                    {
                        count++;
                        //Resend FIN!
                        //If send fails (returns -1) terminate else move on.
                        jail(&jailList, outputBuf, sock, serverAddr, genError);
                        /*
                        if (sendto(sock, &outputBuf, sizeof(Package), 0, (struct sockaddr *) &serverAddr, slen) == -1) {
                            die("sendto()");
                        }*/

                        printf("-=RESENDING FIN!=-");
                        printPackage(outputBuf);
                    }
                    else
                    {
                        uint contFlag = 0;
                        contFlag = yesNoRepeater("Safe shutdown fail?");
                        if(contFlag)
                        {
                            //Try again!
                            currentState = INITCLOSE;
                        }
                        else
                        {
                            //Terminate!
                            exit(EXIT_FAILURE);
                        }
                    }
                }



                Node *ptr = freeFromJail(&jailList);
                if(ptr != NULL)
                {
                    if (sendto(sock, &ptr->data, sizeof(Package), 0, (struct sockaddr *) &serverAddr, slen) == -1) {
                        perror("sendto()");
                        exit(1);
                        // die("sendto()"); // fails
                    }
                    printf("\n-=JAIL RELEASE=-");
                    printPackage(ptr->data);
                    free(ptr);
                    ptr = NULL;
                }


                break;
            }
            case WAITINGCLOSE:
            {
                //WAITING FOR RESENDS OF FIN+ACK!
                readFdSet = activeFdSet;
                timeout_t.tv_sec = 40; // Time that we listen for acks b4 resending the window.
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

                    // Errortracing Code!
                    printf("\n-=Input package=-");
                    printPackage(inputBuf);

                    packType = viewPackage(inputBuf);
                    //If the package contains data!
                    if (packType == 4 && inputBuf.ack == incMaxAck - 1) {
                        //If send fails (returns -1) terminate else move on.
                        jail(&jailList, outputBuf, sock, serverAddr, genError);
                        /*
                        if (sendto(sock, &outputBuf, sizeof(Package), 0, (struct sockaddr *) &serverAddr, slen) == -1) {
                            die("sendto()");
                        }
                        */
                        printf("-=RESEND OF LAST ACK!=-");
                        printPackage(outputBuf);
                    }
                    else
                    {
                        printf("\nDID NOT RECIEVE EXPECTED RESEND OF FinACK!");
                    }
                }
                else
                {
                    printf("\n\nREACHED CLOSED!!!");
                    currentState = CLOSED;
                }



                trigger = rand() %100;


                if(trigger > RELEASEPROBABILITY)// jail release probablity
                {
                    Node *ptr = freeFromJail(&jailList);
                    if(ptr != NULL)
                    {
                        if (sendto(sock, &ptr->data, sizeof(Package), 0, (struct sockaddr *) &serverAddr, slen) == -1) {
                            perror("sendto()");
                            exit(1);
                            // die("sendto()"); // fails
                        }
                        printf("\n-=JAIL RELEASE=-");
                        printPackage(ptr->data);
                        free(ptr);
                        ptr = NULL;
                    }
                }


                break;
            }
            case CLOSED:
            {
                printf("\nWE ARE DONE!");
                fclose(fp);
                return 0;
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