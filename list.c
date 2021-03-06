#include "list.h"
#include "Generic.h"


/* Skriv era funktionsimplementationer för interfacet till er länkade lista här 
   Det går också bra att skriva statiska hjälpfunktioner i denna fil            */




/* Skriv era funktionsimplementationer för interfacet till er länkade lista här
Det går också bra att skriva statiska hjälpfunktioner i denna fil            */

List *createList()
{
    List *newList = (List*)malloc(sizeof(List));

    if (newList == NULL)
    {
        printf("\nYou done goofed");
        return NULL;
    }
    newList->head = NULL;
    printf("List created!");
    return newList;
}

void removeList(List* myList)
{
    Node *current = myList->head;
    Node *previous = current;
    while (current != NULL)
    {
        previous = current;
        current = current->Next;
        free(previous);
    }
}

void removeFirst(List* myList)
{
    Node *current = myList->head;
    Node *previous = current;
    if (current != NULL)
    {
        current = current->Next;
        free(previous);
        previous = NULL;
        myList->head = current;
    }
}

Node *createNode(const Package pack)
{
    Node *newNode = (Node*)malloc(sizeof(Node));
    if (newNode == NULL)
    {
        printf("Memory for new node could not be allocated");
        return NULL;
    }
   newNode->data = pack; // create empty pack?

    newNode->Next = NULL;
    return newNode;
}

void addNodeLast(List *list, Package dataInput)
{
    if (list->head == NULL)
    {
        list->head = createNode(dataInput);
        return;
    }
    Node *current = list->head;
    Node *previous = current;

    while (current != NULL)
    {
        previous = current;
        current = current->Next;
    }

    previous->Next = createNode(dataInput);

}

Node* retrieveLastNode(List list)
{
    if(list.head != NULL)
    {
        Node *current = list.head;
        while (current->Next != NULL) {
            current = current->Next;
        }
        return current;
    }
    else
    {
        return NULL;
    }
}

void addNodeFirst(List *list, Package dataInput) // will work on head being NULL as it will just overwrite the nodes->Next NULL with NULL
{
    Node *temp = list->head;
    Node *iter;

    list->head = createNode(dataInput);
    iter = list->head;
    iter->Next = temp;
}

int numberOfNodes(List *list)
{
    int x = 0;
    Node *iter = list->head;

    while (iter != NULL)
    {
        x++;
        iter = iter->Next;
    }
    return x;
}
/* not fixed
int dataSearch(List *list, Package x)
{
    Node *current = list->head;
    int flag = 0;

    while (current != NULL)
    {
        if (current->data == x) flag = 1;
        current = current->Next;
    }

    return flag;
}
*/

int IsListEmpty(List *list)
{

    if (numberOfNodes(list) == 0)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

void printList(List *mylist)
{
    Node *iterator = mylist->head;
    if (numberOfNodes(mylist) == 0)
    {
        printf("\nThis list is empty you scrub");
    }
    else
    {
        printf("\nThelist: ");
        do
        {
          //  printf("%d   ", iterator->data); fix printing
            printPackage(iterator->data);
            iterator = iterator->Next;
        } while (iterator != NULL);
    }
}



void jail(List *jailList, Package pack, int sock, struct sockaddr_in serverAddr, bool genError)
{
    uint8_t trigger = rand() % 100;
    if(genError == false)
    {
        trigger = 0;
    }
    if(trigger > ERRORPROBABILITY)
    {
        uint8_t errorType = rand() % 2;

        //destroy checksum
        if (errorType == 0)
        {
            addNodeLast(jailList, pack);
            pack.checkSum = pack.checkSum + 1;
            if (sendto(sock, &pack, sizeof(Package), 0, (struct sockaddr*) &serverAddr, sizeof(serverAddr)) == -1)
            {
                die("sendto()");
            }
        }
        else if (errorType == 1)
        {
            //Jail package
            addNodeLast(jailList, pack);
        }
        else
        {
            printf("ShitFuck! errorType too big");
        }
    }
    else
    {
        if (sendto(sock, &pack, sizeof(Package), 0, (struct sockaddr*) &serverAddr, sizeof(serverAddr)) == -1)
        {
            die("sendto()");
        }
    }
}

/*
Node *ptr = freeFromJail(list);
if(ptr != NULL)
{
//send
free(ptr);
}
*/
Node* freeFromJail(List*list)
{
    Node *curr = list->head;
    Node *prev = curr;
    int numNodes = numberOfNodes(list);

    int count = 0;
    //somerandom numbbetween0andlength - 1

    if(numNodes == 0)
    {
        return NULL;
    }
    int targetNode = rand() % numberOfNodes(list); // must be at least 2 elements in list
    if(targetNode == 0)
    {
        list->head = curr->Next;
        return curr;
    }

    while (count < targetNode)  // vi har 2 nodes, targetnode = 1,
    {
        prev = curr;
        curr = curr->Next;
        count++;
    }

    prev->Next = curr->Next;
    return curr;
}


