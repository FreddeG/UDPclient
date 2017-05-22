#include "list.h"
#include "Generic.h"


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


// This is the semiList-function that we use for sending packages with errors packet lost (eg goes to jail),
// checksum corrupted. The jailList is used for storing all packages that were have been jailed so that we can simulate
// old and unordered packages.
// The package becomes a local copy of outputbuf (from main) and genError is a flag that turns the error generating code off.
void jail(List *jailList, Package pack, int sock, struct sockaddr_in serverAddr, bool genError)
{
    //create the trigger value
    uint8_t trigger = rand() % 100;
    printf("\n Trigger is:%uz", trigger);
    if(genError == false)
    {
        trigger = 0;
    }
    //compare trigger with the treshold value in ERRORPROBABILITY
    if(trigger < ERRORPROBABILITY)
    {
        //Pick a errortype at random.
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
            printf("\n!!Destroying checksum!!\n");
            printPackage(pack);
        }
        else if (errorType == 1)
        {
            printf("\n!!GOES TO JAIL!!");
            printPackage(pack);
            addNodeLast(jailList, pack);
        }
        else
        {
            printf("ShitFuck! errorType too big");
        }
    }
    else
    {
        //If no error is generated (trigger to high) we send the package as usual.
        if (sendto(sock, &pack, sizeof(Package), 0, (struct sockaddr*) &serverAddr, sizeof(serverAddr)) == -1)
        {
            die("sendto()");
        }
        printf("\n-=Output package=-");
        printPackage(pack);
    }
}

// Frees packages from jail. This simulates old packages and dubbel sends and packages out of order.
Node* freeFromJail(List*list)
{
    Node *curr = list->head;
    Node *prev = curr;
    int numNodes = numberOfNodes(list);

    int count = 0;

    if(numNodes == 0)
    {
        return NULL;
    }
    //Random node between 0 and number of nodes - 1.
    int targetNode = rand() % numberOfNodes(list); // reached here: at least 2 elements in list
    if(targetNode == 0)
    {
        list->head = curr->Next;
        return curr;
    }

    while (count < targetNode)  // if 2 nodes, targetnode = 1 is the last node,
    {
        prev = curr;
        curr = curr->Next;
        count++;
    }

    //relink the list and return the lose node.
    prev->Next = curr->Next;
    return curr;
}


