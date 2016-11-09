#ifndef __DEFAULTOPS_H__
#define __DEFAULTOPS_H__

typedef struct node{
    char* s;
    struct node* next;
} string;

string *end;
string *first;

void insert(char *ss) {
    string *new = malloc(sizeof(string));
    new -> s = estrdup(ss);
    new -> next = NULL;
    end -> next = new;
    end = end -> next;
}

#endif