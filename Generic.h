//
// Created by kajvi on 2017-05-15.
//

#ifndef UDPCLIENT_GENERIC_H
#define UDPCLIENT_GENERIC_H

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#define SERVER "127.0.0.1" // use gethostbyname // getaddrinfo
#define PORT 8888   //The port on which to send data
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
    bool syn;
    uint64_t seq;
    uint64_t ack;
    uint16_t winSize;
    char data;
    uint64_t checkSum;
} Package;


static void flushRestOfLine(void);
uint yesNoRepeater(char ir_prompt[]);
void die(char *s);
void emptyPackage(Package *packToEmpty);
void printPackage(Package pack);
uint8_t viewPackage(Package pack);
uint64_t initSEQ(void);
uint64_t checksum (Package pack);
bool checksumChecker(Package pack);



#endif //UDPCLIENT_GENERIC_H
