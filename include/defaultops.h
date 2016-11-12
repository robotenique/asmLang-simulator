#ifndef __DEFAULTOPS_H__
#define __DEFAULTOPS_H__

typedef enum {false, true} bool;

typedef struct line {
    char *line;
    int number;
    struct line* next;
} Line;

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
