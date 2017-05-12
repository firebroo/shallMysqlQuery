#include "hashtable.h"

static inline void __free_value__ (HashNode *node);
static void __free_hashnode__ (HashNode *node);
static inline void __hash_rehash__ (HashNode **newHashNode, \
        size_t pos, HashNode *pHead,  HashNode* pOldHead);
static void __hash_table_rehash__ (HashTable *hashtable);
static HashNode* __deep_clone_hashnode__ (HashNode* hashnode);
static void __free_hlist__(HashNode *node);

void
__free_hlist__(HashNode *pHead)
{
    HashNode  *tmp;

    while (pHead) {
        tmp = pHead->pNext;
        __free_hashnode__(pHead);
        pHead = tmp;
    }
}

/*if key is exists and type is STRING, free string's value memory*/
inline void 
__free_value__ (HashNode *node)
{
    if (node->type == STRING) free(zStrValue(node));
    if (node->type == VOID) free(zVoidValue(node));
}

/* copy current node, set the next pointer point NULL */
HashNode* 
__deep_clone_hashnode__ (HashNode* hashnode) {
    HashNode *clonedHashnode = (HashNode *)malloc(sizeof(HashNode)); 
    *clonedHashnode = *hashnode; 
    clonedHashnode->pNext = NULL;                                  
    return clonedHashnode;
}                    

void
__hash_rehash__ (HashNode **newHashNode, size_t pos, \
        HashNode* pHead,  HashNode* pInsertNode)
{
    HashNode  *pNewLast;

    if (pHead) {
        do {
            pNewLast = pHead;
        } while ((pHead = pHead->pNext));
        pNewLast->pNext = pInsertNode;
    } else {
        newHashNode[pos] = pInsertNode; 
    }
}

void
__hash_table_rehash__ (HashTable *hashtable)
{
    size_t     i, pos, hash_old_size;
    HashNode   *pOldHead, **newHashNode, *tmp;

    hash_old_size = hashTable_max_size(hashtable);
    hashTable_expand(hashtable);
    newHashNode = (HashNode **)calloc (sizeof(HashNode*), \
        hashTable_max_size(hashtable));
    for (i = 0; i < hash_old_size; i++) {
        if ((pOldHead = (hashtable->hashnode)[i])) {
            while (pOldHead) {
                tmp = pOldHead->pNext;
                pos = hash_pos (pOldHead->hash, hashtable);
                pOldHead->pNext = NULL;
                __hash_rehash__ (newHashNode, pos, newHashNode[pos], pOldHead);
                pOldHead = tmp;
            }
        }
    }
    free (hashtable->hashnode);
    hashtable->hashnode = newHashNode;
}

void
__free_hashnode__ (HashNode *node)
{
    free (node->sKey);          /* free key */
    __free_value__ (node);      /* if value is string, free value */
    free (node->pValue);        /* free zvalue struct */
    free (node);                /* free current hashnode */
}

/* initialize hash table */
void
hash_table_init (HashTable *hashtable)
{
    hashtable->hash_table_max_size = HASH_TABLE_INIT_SIZE;
    hashtable->hash_size = 0;
    hashtable->hashnode = (HashNode **)calloc (sizeof(HashNode*), \
            HASH_TABLE_INIT_SIZE);
}

#if 0
/* string hash function */
unsigned int
hash_table_hash_str (const char* skey)
{
    /*  
     * This is the popular `times 33' hash algorithm which is used by  
     * perl and also appears in Berkeley DB. This is one of the best  
     * known hash functions for strings because it is both computed  
     * very fast and distributes very well.  
     *  
     * The originator may be Dan Bernstein but the code in Berkeley DB  
     * cites Chris Torek as the source. The best citation I have found  
     * is "Chris Torek, Hash function for text in C, Usenet message  
     * <27038@mimsy.umd.edu> in comp.lang.c , October, 1990." in Rich  
     * Salz's USENIX 1992 paper about INN which can be found at  
     * <http://citeseer.nj.nec.com/salz92internetnews.html>.  
     *  
     * The magic of number 33, i.e. why it works better than many other  
     * constants, prime or not, has never been adequately explained by  
     * anyone. So I try an explanation: if one experimentally tests all  
     * multipliers between 1 and 256 (as I did while writing a low-level  
     * data structure library some time ago) one detects that even  
     * numbers are not useable at all. The remaining 128 odd numbers  
     * (except for the number 1) work more or less all equally well.  
     * They all distribute in an acceptable way and this way fill a hash  
     * table with an average percent of approx. 86%.  
     *  
     * If one compares the chi^2 values of the variants (see  
     * Bob Jenkins ``Hashing Frequently Asked Questions'' at  
     * http://burtleburtle.net/bob/hash/hashfaq.html for a description  
     * of chi^2), the number 33 not even has the best value. But the  
     * number 33 and a few other equally good numbers like 17, 31, 63,  
     * 127 and 129 have nevertheless a great advantage to the remaining  
     * numbers in the large set of possible multipliers: their multiply  
     * operation can be replaced by a faster operation based on just one  
     * shift plus either a single addition or subtraction operation. And  
     * because a hash function has to both distribute good _and_ has to  
     * be very fast to compute, those few numbers should be preferred.  
     *  
     *                  -- Ralf S. Engelschall <rse@engelschall.com>  
     */   
    const char   *p;
    unsigned int  hash;

    p = (const char*)skey;
    hash = *p;
    if (hash) {
        for (p += 1; *p != '\0'; p++)
            hash = (hash << 5) - hash + *p;
    }
    return hash;
}
#endif

inline unsigned long 
hash_func (const char *arKey)
{
    size_t nKeyLength = strlen(arKey);
    register unsigned long hash = 5381;

    /* variant with the hash unrolled eight times */
    for (; nKeyLength >= 8; nKeyLength -= 8) {
        hash = ((hash << 5) + hash) + *arKey++;
        hash = ((hash << 5) + hash) + *arKey++;
        hash = ((hash << 5) + hash) + *arKey++;
        hash = ((hash << 5) + hash) + *arKey++;
        hash = ((hash << 5) + hash) + *arKey++;
        hash = ((hash << 5) + hash) + *arKey++;
        hash = ((hash << 5) + hash) + *arKey++;
        hash = ((hash << 5) + hash) + *arKey++;
    }
    switch (nKeyLength) {
        case 7: hash = ((hash << 5) + hash) + *arKey++; /* fallthrough... */
        case 6: hash = ((hash << 5) + hash) + *arKey++; /* fallthrough... */
        case 5: hash = ((hash << 5) + hash) + *arKey++; /* fallthrough... */
        case 4: hash = ((hash << 5) + hash) + *arKey++; /* fallthrough... */
        case 3: hash = ((hash << 5) + hash) + *arKey++; /* fallthrough... */
        case 2: hash = ((hash << 5) + hash) + *arKey++; /* fallthrough... */
        case 1: hash = ((hash << 5) + hash) + *arKey++; break;
        case 0: break;
    }

    return hash;
}

void
hash_table_insert_struct (HashTable *hashtable, const char* skey, void *pvalue)
{
    size_t          pos;
    HashNode       *pHead, *pLast;
    HashNode       *pNewNode;
    unsigned long   hashKey;

    if (hash_table_is_rehashing(hashtable)) {
        __hash_table_rehash__(hashtable);
    }

    pHead = pLast = NULL;
    hashKey = hash(skey);
    pos = hash_pos (hashKey, hashtable);
    pHead = (hashtable->hashnode)[pos];
    if (pHead) {
        while (pHead) {
            if (strcmp (pHead->sKey, skey) == 0) {
                __free_value__ (pHead);
                r_set_zVoidval (pHead, pvalue);
            }
            pLast = pHead;
            pHead = pHead->pNext;
        }
    }
    pNewNode = (HashNode*) malloc (sizeof (HashNode));
    set_key (pNewNode, skey);
    pNewNode->pValue = (void *) malloc (sizeof(zValue));
    set_zVoidval (pNewNode, pvalue);
    pNewNode->pNext = NULL;
    pNewNode->hash = hashKey;

    if (pLast) {
        pLast->pNext = pNewNode;
    } else {
        (hashtable->hashnode)[pos] = pNewNode;
    }

}


/*insert key-value into hash table, if key is exist, 
 *it will overwrite old value, use Linked list to slove 
 *hash conflict.*/
void
hash_table_insert_long (HashTable *hashtable, const char* skey, long nvalue)
{
    size_t          pos;
    HashNode       *pHead, *pLast;
    HashNode       *pNewNode;
    unsigned long   hashKey;

    if (hash_table_is_rehashing(hashtable)) {
        __hash_table_rehash__(hashtable);
    }

    pHead = pLast = NULL;
    hashKey = hash(skey);
    pos = hash_pos (hashKey, hashtable);
    pHead = (hashtable->hashnode)[pos];
    if (pHead) {
        while (pHead) {
            if (strcmp (pHead->sKey, skey) == 0) {
                __free_value__ (pHead);
                r_set_zlval (pHead, nvalue);
            }
            pLast = pHead;
            pHead = pHead->pNext;
        }
    }
    pNewNode = (HashNode*) malloc (sizeof (HashNode));
    set_key (pNewNode, skey);
    pNewNode->pValue = (void *) malloc (sizeof(zValue));
    set_zlval (pNewNode, nvalue);
    pNewNode->pNext = NULL;
    pNewNode->hash = hashKey;

    if (pLast) {
        pLast->pNext = pNewNode;
    } else {
        (hashtable->hashnode)[pos] = pNewNode;
    }
    hashtable->hash_size++;
}

void
hash_table_insert_string (HashTable *hashtable, const char* skey, char* pValue)
{
    size_t          pos;
    HashNode       *pNewNode;
    HashNode       *pHead, *pLast;
    unsigned long   hashKey;

    if (hash_table_is_rehashing(hashtable)) {
        __hash_table_rehash__(hashtable);
    }

    pHead = pLast = NULL;
    hashKey = hash(skey);
    pos = hash_pos (hashKey, hashtable);
    pHead = (hashtable->hashnode)[pos];
    if (pHead) {
        while (pHead) {
            if (strcmp (pHead->sKey, skey) == 0) {
                __free_value__ (pHead);
                r_set_zStrval (pHead, pValue);      /*set then return*/
            }
            pLast = pHead;                          /*lastest hashnode*/
            pHead = pHead->pNext;
        }
    } 
    pNewNode = (HashNode*) malloc (sizeof (HashNode));
    set_key (pNewNode, skey);
    pNewNode->pValue = (zValue *) malloc (sizeof (zValue));
    set_zStrval (pNewNode, pValue);
    pNewNode->pNext = NULL;
    pNewNode->hash = hashKey;
    if (pLast) {
        pLast->pNext = pNewNode;
    } else {
        (hashtable->hashnode)[pos] = pNewNode;
    }
    hashtable->hash_size++;
}

void
hash_table_insert_bool (HashTable *hashtable, const char* skey, bool value)
{
    size_t          pos;
    HashNode       *pHead, *pLast;
    HashNode       *pNewNode;
    unsigned long   hashKey;

    if (hash_table_is_rehashing(hashtable)) {
        __hash_table_rehash__(hashtable);
    }

    pHead = pLast = NULL;
    hashKey = hash(skey);
    pos = hash_pos (hashKey, hashtable);
    pHead =  (hashtable->hashnode)[pos];
    if (pHead) {
        while (pHead) {
            if (strcmp (pHead->sKey, skey) == 0) {
                __free_value__ (pHead);
                r_set_zbval (pHead, value);
            }
            pLast = pHead;
            pHead = pHead->pNext;
        }
    }
    pNewNode = (HashNode*) malloc (sizeof (HashNode));
    set_key (pNewNode, skey);
    pNewNode->pValue = (zValue *) malloc (sizeof(zValue));
    set_zbval (pNewNode, value);
    pNewNode->hash = hashKey;
    pNewNode->pNext = NULL;
    if (pLast) {
        pLast->pNext = pNewNode;
    } else {
        (hashtable->hashnode)[pos] = pNewNode;
    }

    hashtable->hash_size++;
}

void
hash_table_insert_double (HashTable *hashtable, const char* skey, double dval)
{
    size_t          pos;
    HashNode       *pHead, *pLast;
    HashNode       *pNewNode;
    unsigned long   hashKey;

    if (hash_table_is_rehashing(hashtable)) {
        __hash_table_rehash__(hashtable);
    }

    pHead = pLast = NULL;
    hashKey = hash(skey);
    pos = hash_pos (hashKey, hashtable);
    pHead = (hashtable->hashnode)[pos];
    if (pHead) {
        while (pHead) {
            if (strcmp (pHead->sKey, skey) == 0) {
                __free_value__ (pHead);
                r_set_zdval (pHead, dval);
            }
            pLast = pHead;
            pHead = pHead->pNext;
        }
    }
    pNewNode = (HashNode*) malloc (sizeof (HashNode));
    set_key (pNewNode, skey);
    pNewNode->pValue = (zValue *) malloc (sizeof(zValue));
    set_zdval (pNewNode, dval);
    pNewNode->pNext = NULL;
    pNewNode->hash = hashKey;
    if (pLast) {
        pLast->pNext = pNewNode;
    } else {
        (hashtable->hashnode)[pos] = pNewNode;
    }

    hashtable->hash_size++;
}

/*remove key-value from the hash table*/
void
hash_table_remove (HashTable *hashtable, const char* skey)
{
    size_t     pos;
    HashNode  *pHead, *pLast, *pRemove;

    pHead = pLast = pRemove = NULL;
    pos = hash_pos (hash(skey), hashtable);
    if ( (pHead = hashtable->hashnode[pos]) ) {
        hashNode_for_each (pHead) {
            if (strcmp (skey, pHead->sKey) == 0) {
                pRemove = pHead;
                break;
            }
            pLast = pHead;
        }
        if (pRemove) {
            if (pLast) {
                pLast->pNext = pRemove->pNext;
            } else {
                hashtable->hashnode[pos] = pRemove->pNext;
            }
            __free_hashnode__ (pRemove);
        } else {
            printf ("key is not exist.\n");
        }
    } else {
        printf ("key is not exist.\n");
    }
}

/* lookup a key in the hash table */
HashNode*
hash_table_lookup (HashTable *hashtable, const char* skey)
{
    unsigned int    pos;
    HashNode        *pHead;

    pos = hash_pos (hash(skey), hashtable);
    if ( (pHead = (hashtable->hashnode)[pos]) ) {
        hashNode_for_each (pHead) {
            if (strcmp (skey, pHead->sKey) == 0)
                return pHead;
        }
    }
    return NULL;
}

void
hash_node_print (HashNode *hashnode) {
    switch (hashnode->type) {

    case LONG:
        printf ("%s(len=%ld) => %ld :: char* -> long\n", hashnode->sKey, strlen(hashnode->sKey), 
                zlval (hashnode));
        break;
    case STRING:
        printf ("%s(len=%ld) => %s(length=%ld) :: char* -> char*\n", 
                hashnode->sKey, strlen(hashnode->sKey), zStrValue(hashnode), zStrLen(hashnode));
        break;
    case BOOL:
        printf ("%s(len=%ld) => %s :: char* -> bool\n", hashnode->sKey, strlen(hashnode->sKey), 
                Bool(zlval(hashnode))? "true": "false");
        break;
    case DOUBLE:
        printf ("%s(len=%ld) => %lf :: char* -> double\n", hashnode->sKey, strlen(hashnode->sKey),
                zdval (hashnode));
        break;
    case VOID:
        printf ("%s(len=%ld) => %p :: char* -> void*\n", hashnode->sKey, strlen(hashnode->sKey),
                (zVoidValue (hashnode)));
    default:
        break;
    }
}
         

/* print the content in the hash table */
void 
hash_table_print (HashTable *hashtable)
{
    size_t     i;
    HashNode  *pHead; 

    for (i = 0; i < hashtable->hash_table_max_size; i++) {
        if ( (pHead = (hashtable->hashnode)[i]) ) {
            hashNode_for_each (pHead) {
                hash_node_print (pHead);
            }
        }
    }
}

/* free the memory of the hash table */
void
hash_table_release (HashTable *hashtable)
{
    size_t      i;
    HashNode   *tmp;

    for (i = 0; i < hashtable->hash_table_max_size; i++) {
        __free_hlist__((hashtable->hashnode)[i]);
    }
    free (hashtable->hashnode);
}
