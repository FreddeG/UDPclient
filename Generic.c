//
// Created by kajvi on 2017-05-15.
//
#include "Generic.h"



static void flushRestOfLine(void)
{
    // Call ONLY when EOL expected from the keyboardstream.

    char ch;
    do
    {
        scanf("%c", &ch);
        //printf_s("\n%d\n", ch);
    } while (ch != '\n');
}// flushRestOfLine

// ============================================================================

// Print prompt: wait for "y" or "n" to be entered and return 1 = yes , 0 = no
uint yesNoRepeater(char ir_prompt[])
{
    char repeatChar;
    do
    {
        // Loopar tills man väljer y eller n.

        printf("%s (y/n): ", ir_prompt);
        scanf("%c", &repeatChar);

        // Tömmer resten av In-strömmen från tangentbordet fram till radslut.
        flushRestOfLine();

        if (repeatChar == 'y' || repeatChar == 'Y')
        {
            return 1;
        }
        else if (repeatChar == 'n' || repeatChar == 'N')
        {
            return 0;
        }
        else
        {
            printf("Please enter either y (yes) or n (no)\n");
        }
    } while (1);
} // yesNoRepeater



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


uint8_t viewPackage(Package pack)
{

    if(checksum(pack)) // checksum
    {
        if(pack.syn == true) return 1; // syn

        if(pack.fin == true) return 4; // last package

        if(pack.reset == true) return 5; // server resets connection

        if(pack.data == '\0') return 2; // ack + seq, no data

        if(pack.data != '\0') return 3; // ack + seq with data, package to read

        printf("\n ERROR!");
        exit(1);

    }
    else
    {
        return 0;
    }

    //checksum wrong return
    // 0 bad case, (checksum fail)
    // 1 syn (seq, no data, no ack)
    // 2 ack (seq + ack, no data)
    // 3 data (seq + ack + data)
    // 4 fin (fin = true, rest doesn't matter)
    // 5 reset

}


uint64_t initSEQ(void)
{
    struct timeval te;
    gettimeofday(&te, NULL); // get current time
    uint64_t milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // caculate milliseconds
    // printf("milliseconds: %lld\n", milliseconds);
    return milliseconds;
}


uint64_t checksum (Package pack)
{
    unsigned char *ptr=(unsigned char *)&pack;
    int sizestruct = sizeof(Package);
    uint64_t chksm = 0;
    int i;

    for(i=0; i<sizestruct; i++)
        chksm = chksm + ptr[i];


    return chksm;
}

bool checksumChecker(Package pack)
{
    uint64_t stateVal = pack.checkSum;
    pack.checkSum = 0;
    uint64_t calcVal = checksum(pack);

    if(stateVal == calcVal)
    {
        return true;
    }
    else
    {
        return false;
    }
}

