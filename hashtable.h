#ifndef HASHTABLE_H
#define HASHTABLE_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define HASH_TABLE_INIT_SIZE (1 << 8) 

#define hash(skey) \
    (hash_func(skey))

#define hash_pos(hashKey, hashtable) \
    ((hashKey) % (hashtable->hash_table_max_size))

#define hashTable_is_full(hashtable) \
    (((hashtable)->hash_size) > ((hashtable)->hash_table_max_size))

#define hashTable_is_empty(hashtable) \
    (((hashtable)->hash_size) == 0)

#define hashNode_for_each(hashnode) \
    for (; (hashnode); (hashnode) = (hashnode)->pNext)

#define hashTable_size(hashtable) \
    ((hashtable)->hash_size)

#define hashTable_max_size(hashtable) \
    ((hashtable)->hash_table_max_size)

#define hashTable_expand(hashtable) \
    ((hashTable_max_size(hashtable)) <<= 1)

#define zvalue(hashnode) \
    (((zValue *)(hashnode)->pValue)->value)

#define zVoidValue(hashnode) \
    ((zvalue(hashnode)).pval)

#define zStrValue(hashnode) \
    ((zvalue(hashnode)).str.value)

#define zStrLen(hashnode) \
    ((zvalue(hashnode)).str.len)

#define zlval(hashnode) \
    ((zvalue(hashnode)).lval)

#define zdval(hashnode) \
    ((zvalue(hashnode)).dval)

#define mallocStr(str) \
    (char*) malloc (strlen (str) + 1)

#define set_zlval(hashnode, lval)           \
    do {                                    \
        zlval ((hashnode)) = lval;          \
        (hashnode)->type   = LONG;          \
    } while(0)

#define set_zVoidval(hashnode, pval)           \
    do {                                    \
        zVoidValue ((hashnode)) = pval;          \
        (hashnode)->type        = VOID;          \
    } while(0)

#define set_key(hashnode, str)              \
    do {                                    \
        (hashnode)->sKey = mallocStr (str); \
        strcpy ((hashnode)->sKey, str);     \
    } while(0)

#define set_zStrval(hashnode, str)                \
    do {                                          \
        zStrValue (hashnode) = mallocStr (str);   \
        strcpy (zStrValue (hashnode), str);       \
        zStrLen (hashnode) = strlen (str);        \
        (hashnode)->type = STRING;                \
    } while(0)

#define Bool(bval) ((bval)? 1: 0)

#define set_zbval(hashnode, bval)      \
    do {                               \
        zlval (hashnode) = Bool(bval); \
        (hashnode)->type = BOOL;       \
    } while(0)

#define set_zdval(hashnode, dval) \
    do {                               \
        zdval (hashnode) = (dval);     \
        (hashnode)->type = DOUBLE;     \
    } while(0)

#define r_set_zVoidval(hashnode, pval)      \
    do {                                  \
        set_zVoidval((hashnode), (pval));   \
        return;                           \
    } while(0)

#define r_set_zStrval(hashnode, str)      \
    do {                                  \
        set_zStrval((hashnode), (str));   \
        return;                           \
    } while(0)

#define r_set_zlval(hashnode, lval)       \
    do {                                  \
        set_zlval((hashnode), (lval));    \
        return;                           \
    } while(0)

#define r_set_zbval(hashnode, bval) \
    do {                                  \
        set_zbval((hashnode), (bval));    \
        return;                           \
    } while(0)

#define r_set_zdval(hashnode, dval)       \
    do {                                  \
        set_zdval((hashnode), (dval));    \
        return;                           \
    } while(0)

#define hash_table_is_rehashing(hashtable) \
    (((hashtable)->hash_size) > ((((hashtable)->hash_table_max_size)*75/100)))


#define hash_table_insert(table, key, value) _Generic((value),\
    _Bool: hash_table_insert_bool, \
    short int: hash_table_insert_long, \
    unsigned short int: hash_table_insert_long, \
    int: hash_table_insert_long, \
    unsigned int: hash_table_insert_long, \
    long int: hash_table_insert_long, \
    unsigned long int: hash_table_insert_long, \
    long long int: hash_table_insert_long, \
    unsigned long long int: hash_table_insert_long, \
    float: hash_table_insert_double, \
    double: hash_table_insert_double, \
    char *: hash_table_insert_string, \
    default: unsupport_type)((table), (key), (value))

typedef enum {
    LONG,
    BOOL,
    STRING,
    DOUBLE,
    VOID,
} Type;

typedef struct zvalue {
    union {
        long    lval;
        double  dval;
        void   *pval;
        struct {
            char *value;
            unsigned long len;
        } str;
    } value;
} zValue;

typedef struct hashnode HashNode;
struct hashnode {
    char          *sKey;
    void          *pValue;
    Type           type;
    HashNode      *pNext;
    unsigned long  hash;
};

typedef struct hashtable {
    HashNode **hashnode;
    size_t     hash_table_max_size;
    size_t     hash_size;
} HashTable;

void hash_table_init (HashTable *hashtable);

/* unsigned int hash_table_hash_str (const char* skey); */

unsigned long hash_func (const char* skey);

void hash_table_insert_long (HashTable *hashtable, 
        const char* skey, long nvalue);

void hash_table_insert_struct (HashTable *hashtable, 
        const char* skey, void *pvalue);

void hash_table_insert_string (HashTable *hashtable, 
        const char* skey, char* pValue);

void hash_table_insert_bool (HashTable *hashtable, 
        const char* skey, bool value);

void hash_table_insert_double (HashTable *hashtable, 
        const char* skey, double dval);

void hash_table_remove (HashTable *hashtable, const char* skey);


HashNode* hash_table_lookup (HashTable *hashtable, const char* skey);

void hash_node_print (HashNode *hashnode);

void hash_table_print (HashTable *hashtable);

void hash_table_release (HashTable *hashtable);

/* it will confirm in comipile time*/
static void
unsupport_type (HashTable *table, const char *key, ...) {
    fprintf(stderr, "unsupport value type, insert key=[%s] failed\n", key);
}

#endif /* !HASHTABLE_H */
