// based on cs3650 starter code

#ifndef SLIST_H
#define SLIST_H

typedef struct slist {
    char* data;
    int   refs;
    struct slist_t* next;
} slist_t;

slist_t* s_cons(const char* text, slist_t* rest);
void   s_free(slist_t* xs);
slist_t* s_explode(const char* text, char delim);


#endif

