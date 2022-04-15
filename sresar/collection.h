#ifndef SRESAR_COLLECTION_H
#define SRESAR_COLLECTION_H

typedef struct _node{
    int           load5s_state;
    int           zoom_state;
    int           load5s;
    double        load1;
    struct _node *next;
    struct _node *prev;
}Node;

void circular_linkedlist_init(Node *plist, int list_size);

#endif
