/*
 * Generated from Zimbu file zimbu_test_zimbu.zu
 */
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdio.h>
#include <strings.h>

/*
 * TYPEDEFS
 */
#ifdef __MINGW32__
typedef long Zint;
typedef long Zbbits;
#else
typedef long long Zint;
typedef long long Zbbits;
#endif
typedef int Zbits;
typedef int Zbool;
typedef int Zstatus;
typedef int Zenum;
typedef struct Zoref__S Zoref;
typedef struct Zfor__S Zfor_T;

typedef struct CListHead__S CListHead;
typedef struct CListItem__S CListItem;

typedef unsigned long Zhashtype;
typedef struct CDictItem__S CDictItem;
typedef struct CDictHead__S CDictHead;

typedef struct MIO__CStat__S MIO__CStat;

#ifndef INC_builtin_T
#include "ZUDIR/builtin.t.h"
#endif
#ifndef INC_config_T
#include "ZUDIR/config.t.h"
#endif
#ifndef INC_error_T
#include "ZUDIR/error.t.h"
#endif
#ifndef INC_generate_T
#include "ZUDIR/generate.t.h"
#endif
#ifndef INC_node_T
#include "ZUDIR/node.t.h"
#endif
#ifndef INC_output_T
#include "ZUDIR/output.t.h"
#endif
#ifndef INC_parse_T
#include "ZUDIR/parse.t.h"
#endif
#ifndef INC_scope_T
#include "ZUDIR/scope.t.h"
#endif
#ifndef INC_usedfile_T
#include "ZUDIR/usedfile.t.h"
#endif
#ifndef INC_write_c_T
#include "ZUDIR/write_c.t.h"
#endif
/*
 * STRUCTS
 */
#define MIO__Veof EOF

struct {} dummy;


void *Zalloc(size_t size) {
  return calloc(1, size);
}

void Zerror(char *msg) {
  fprintf(stderr, "ERROR: %s\n", msg);
}

struct Zoref__S {
  char *ptr;
  int  type;
};

struct Zfor__S {
  int type;
  CListItem *listItem;
};

struct CListHead__S {
  CListItem *first;
  CListItem *last;
  int itemCount;
};
struct CListItem__S {
  CListItem *next;
  CListItem *prev;
  union {
    int Ival;
    void *Pval;
  };
  int type;
};

#define HT_INIT_SIZE 16
#define PERTURB_SHIFT 5

#define CDI_FLAG_PVAL 1
#define CDI_FLAG_USED 2
#define CDI_FLAG_DEL 4
struct CDictItem__S {
  Zhashtype hash;
  union {
    Zint Ikey;
    void *Pkey;
  };
  union {
    Zint Ival;
    void *Pval;
  };
  int flags;
};

struct CDictHead__S {
    Zhashtype   mask;
    Zhashtype   used;
    Zhashtype   filled;
    int         locked;
    CDictItem   *array;
    CDictItem   smallArray[HT_INIT_SIZE];
    int         usePkey;
};

Zint MARG__Vcount;

char *MARG__Vname;

char **MARG__Vargs;

struct MIO__CStat__S {
  size_t size;
  long   time;
};
#ifndef INC_builtin_S
#include "ZUDIR/builtin.s.h"
#endif
#ifndef INC_config_S
#include "ZUDIR/config.s.h"
#endif
#ifndef INC_error_S
#include "ZUDIR/error.s.h"
#endif
#ifndef INC_generate_S
#include "ZUDIR/generate.s.h"
#endif
#ifndef INC_node_S
#include "ZUDIR/node.s.h"
#endif
#ifndef INC_output_S
#include "ZUDIR/output.s.h"
#endif
#ifndef INC_parse_S
#include "ZUDIR/parse.s.h"
#endif
#ifndef INC_scope_S
#include "ZUDIR/scope.s.h"
#endif
#ifndef INC_usedfile_S
#include "ZUDIR/usedfile.s.h"
#endif
#ifndef INC_write_c_S
#include "ZUDIR/write_c.s.h"
#endif
/*
 * DECLARE FUNCTIONS AND GLOBALS
 */

Zoref *ZallocZoref(void *ptr, Zint type) {
  Zoref *p = Zalloc(sizeof(Zoref));
  p->ptr = ptr;
  p->type = type;
  return p;
}

int Zstrcmp(char *s1, char *s2) {
  if (s1 == NULL || s2 == NULL) {
    if (s1 == NULL && s2 == NULL)
      return 0;
    if (s1 == NULL)
      return -1;
    return 1;
  }
  return strcmp(s1, s2);
}

char *Zconcat(char *s1, char *s2) {
  char *ss1 = s1 == NULL ? "NULL" : s1;
  char *ss2 = s2 == NULL ? "NULL" : s2;
  char *p = malloc(strlen(ss1) + strlen(ss2) + 1);
  strcpy(p, ss1);
  strcat(p, ss2);
  return p;
}

char *Zbool2string(Zbool n) {
  return n == 0 ? "FALSE" : "TRUE";
}

char *Zint2string(Zint n) {
  char *p = malloc(30);
  sprintf(p, "%lld", n);
  return p;
}

char *Ztochar(Zint n) {
  char *p = malloc(2);
  p[0] = n; p[1] = 0;
  return p;
}

char *Zenum2string(char **names, size_t nm, Zenum n) {
  return (n < 0 || n >= nm) ? "INVALID" : names[n];
}

char *ZStringSlice(char *s, Zint start, Zint end) {
  char *ss = (s == NULL) ? "NULL" : s;
  char *r;
  Zint l = (Zint)strlen(ss);
  Zint is = start < 0 ? l + start : start;
  Zint ie = end < 0 ? l + end : end;
  if (is < 0) is = 0;
  if (is > l) is = l;
  if (ie < is - 1) ie = is - 1;
  if (ie >= l) ie = l - 1;
  l = ie - is + 1;
  r = malloc(l + 1);
  strncpy(r, ss + is, l);
  r[l] = 0;
  return r;
}

char *ZStringByteSlice(char *s, Zint start, Zint end) {
  char *ss = (s == NULL) ? "NULL" : s;
  char *r;
  Zint l = (Zint)strlen(ss);
  Zint is = start < 0 ? l + start : start;
  Zint ie = end < 0 ? l + end : end;
  if (is < 0) is = 0;
  if (is > l) is = l;
  if (ie < is - 1) ie = is - 1;
  if (ie >= l) ie = l - 1;
  l = ie - is + 1;
  r = malloc(l + 1);
  strncpy(r, ss + is, l);
  r[l] = 0;
  return r;
}

Zint ZStringToInt(char *s) {
  Zint r;
  if (s == NULL) return 0;
  sscanf(s, "%lld", &r);
  return r;
}

char *ZStringToLower(char *ss) {
  char *r;
  char *s;
  char *d;
  if (ss == NULL)
    return NULL;
  r = (char *)Zalloc(strlen(ss) + 1);
  s = ss;
  d = r;
  while (*s != 0)
    *d++ = tolower(*s++);
  *d = 0;
  return r;
}

Zint ZStringFind(char *big, char *small) {
  char *r;
  if (big == NULL || small == NULL)
    return -1;
  r = strstr(big, small);
  if (r == NULL)
    return -1;
  return r - big;
}

Zint ZStringQuotedToInt(char *s) {
  Zint r = 0;
  if (s != NULL) {
    char *p;
    for (p = s; *p != 0; ++p) {
      if (*p >= '0' && *p <= '9')
        r = r * 10 + (*p - '0');
      else if (*p != '\'')
        break;
    }
  }
  return r;
}

Zint ZStringQuotedBinToInt(char *s) {
  Zint r = 0;
  if (s != NULL) {
    char *p;
    for (p = s; *p != 0; ++p) {
      if (*p == '0')
        r <<= 1;
      else if (*p == '1')
        r = (r << 1) + 1;
      else if (*p != '\'')
        break;
    }
  }
  return r;
}

Zint ZStringQuotedHexToInt(char *s) {
  Zint r = 0;
  if (s != NULL) {
    char *p;
    for (p = s; *p != 0; ++p) {
      if (*p >= '0' && *p <= '9')
        r = r * 16 + (*p - '0');
      else if (*p >= 'a' && *p <= 'f')
        r = r * 16 + (*p - 'a' + 10);
      else if (*p >= 'A' && *p <= 'F')
        r = r * 16 + (*p - 'A' + 10);
      else if (*p != '\'')
        break;
    }
  }
  return r;
}

Zint ZStringIndex(char *s, Zint c) {
  unsigned char *p;
  if (s == NULL) return -1;
  for (p = (unsigned char *)s; *p != 0; ++p) {
    if (*p == c) return (Zint)((char *)p - s);
  }
  return -1;
}

Zint ZStringRindex(char *s, Zint c) {
  unsigned char *p;
  unsigned char *found;
  if (s == NULL) return -1;
  found = NULL;
  for (p = (unsigned char *)s; *p != 0; ++p) {
    if (*p == c) found = p;
  }
  if (found == NULL) return -1;
  return (int)((char *)found - s);
}

CListItem *ZListFind(CListHead *head, int idx) {
  if (head == NULL) Zerror("Attempt to search in NIL list");
  CListItem *item;
  int n;
  if (idx < 0) {
    item = head->last;
    n = -idx;
    while (--n > 0 && item != NULL)
      item = item->prev;
  } else {
    item = head->first;
    n = idx;
    while (--n >= 0 && item != NULL)
      item = item->next;
  }
  return item;
}

CListHead *ZListExtend(CListHead *head, CListHead *head2) {
  if (head == NULL) Zerror("Attempt to extend NIL list");
  if (head2 == NULL) return head;  /* TODO: throw exception? */
  CListItem *item = head2->first;
  while (item != NULL) {
    CListItem *newitem = Zalloc(sizeof(CListItem));
    if (item->type == 1) {
      newitem->Pval = item->Pval;
      newitem->type = 1;
    } else
      newitem->Ival = item->Ival;
    if (head->last == NULL) {
      head->first = newitem;
      head->last = newitem;
    } else {
      newitem->prev = head->last;
      newitem->prev->next = newitem;
      head->last = newitem;
    }
    ++head->itemCount;
    item = item->next;
  }
  return head;
}

CListHead *ZListAdd(CListHead *head, int after, int nr, void *ptr, int type) {
  if (head == NULL) Zerror("Attempt to append to NIL list");
  CListItem *item = Zalloc(sizeof(CListItem));
  if (type == 0)
    item->Ival = nr;
  else
    item->Pval = ptr;
  item->type = type;
  if (head->last == NULL) {
    head->first = item;
    head->last = item;
  } else {
    CListItem *afterItem = NULL;
    if (after != -1) {
      afterItem = ZListFind(head, after);
    }
    if (afterItem == NULL || afterItem == head->last) {
      item->prev = head->last;
      item->prev->next = item;
      head->last = item;
    } else {
      item->next = afterItem->next;
      item->next->prev = item;
      item->prev = afterItem;
      afterItem->next = item;
    }
  }
  ++head->itemCount;
  return head;
}

CListHead *ZListInsert(CListHead *head, int before, int nr, void *ptr, int type) {
  if (head == NULL) Zerror("Attempt to insert in NIL list");
  CListItem *item;
  CListItem *iitem;
  if (head->first == NULL)
    return ZListAdd(head, -1, nr, ptr, type);
  iitem = ZListFind(head, before);
  if (iitem == NULL) {
    if (before >= 0)
      return ZListAdd(head, -1, nr, ptr, type);
    iitem = head->first;
  }
  item = Zalloc(sizeof(CListItem));
  if (type == 0)
    item->Ival = nr;
  else
    item->Pval = ptr;
  item->type = type;
  item->prev = iitem->prev;
  item->next = iitem;
  if (item->prev == NULL)
    head->first = item;
  else
    item->prev->next = item;
  item->next->prev = item;
  ++head->itemCount;
  return head;
}

CListHead *ZListClear(CListHead *head) {
  if (head == NULL) Zerror("Attempt to clear NIL list");
  head->first = NULL;  /* TODO: free items */
  head->last = NULL;
  head->itemCount = 0;
  return head;
}

int ZListGetInt(CListHead *head, int idx) {
  CListItem *item = ZListFind(head, idx);
  if (item != NULL)
    return item->Ival;
  return 0;
}

int *ZListGetIntP(CListHead *head, int idx) {
  CListItem *item = ZListFind(head, idx);
  if (item != NULL)
    return &item->Ival;
  return NULL;
}

void *ZListGetPtr(CListHead *head, int idx) {
  CListItem *item = ZListFind(head, idx);
  if (item != NULL)
    return item->Pval;
  return NULL;
}

int ZListPopIntItem(CListHead *head, int idx) {
  CListItem *item = ZListFind(head, idx);
  if (item != NULL) {
    int v = item->Ival;
    if (item->prev == NULL)
      head->first = item->next;
    else
      item->prev->next = item->next;
    if (item->next == NULL)
      head->last = item->prev;
    else
      item->next->prev = item->prev;
    /* TODO: free *item */
    head->itemCount--;
    return v;
  }
  return 0;
}

void *ZListPopPtrItem(CListHead *head, int idx) {
  CListItem *item = ZListFind(head, idx);
  if (item != NULL) {
    void *p = item->Pval;
    if (item->prev == NULL)
      head->first = item->next;
    else
      item->prev->next = item->next;
    if (item->next == NULL)
      head->last = item->prev;
    else
      item->next->prev = item->prev;
    /* TODO: free *item */
    head->itemCount--;
    return p;
  }
  return NULL;
}

CDictHead *ZnewDict(int usePkey) {
  CDictHead *d = Zalloc(sizeof(CDictHead));
  d->array = d->smallArray;
  d->mask = HT_INIT_SIZE - 1;
  d->usePkey = usePkey;
  return d;
}

Zhashtype ZDictHash(CDictHead *d, Zint Ikey, unsigned char *Pkey) {
  if (d->usePkey) {
    Zhashtype hash = *Pkey;
    if (hash != 0) {
      unsigned char *p = Pkey + 1;
      while (*p != 0)
        hash = hash * 101 + *p++;
    }
    return hash;
  }
  return (Zhashtype)Ikey;
}

/* #define DICT_DEBUG 1 */

CDictItem *ZDictLookup(CDictHead *d, Zint Ikey, void *Pkey, Zhashtype hash)
{
  Zhashtype  perturb;
  CDictItem  *freeitem;
  CDictItem  *di;
  int        idx;
#ifdef DICT_DEBUG
  int        echo = Pkey != NULL && strcmp(Pkey, "EQUAL") == 0;
#endif

  idx = (int)(hash & d->mask);
  di = &d->array[idx];
#ifdef DICT_DEBUG
  if (echo)
    printf("idx = %d, flags = %d, key = %s, value = %lld\n",
    idx, di->flags, (char *)di->Pkey, di->Ival);
#endif

  if (di->flags == 0)
    return di;
  if (di->flags == CDI_FLAG_DEL)
    freeitem = di;
  /* TODO: other keys than string */
  else if (di->hash == hash && (d->usePkey
       ? (di->Pkey != NULL && Pkey != NULL && strcmp(di->Pkey, Pkey) == 0)
       : di->Ikey == Ikey)) {
#ifdef DICT_DEBUG
    if (echo) printf("found it\n");
#endif
    return di;
  } else {
    freeitem = NULL;
  }

  for (perturb = hash; ; perturb >>= PERTURB_SHIFT)
  {
    idx = (int)((idx << 2) + idx + perturb + 1);
    di = &d->array[idx & d->mask];
#ifdef DICT_DEBUG
    if (echo)
      printf("idx = %d, flags = %d\n", idx, di->flags);
#endif
    if (di->flags == 0)
      return freeitem == NULL ? di : freeitem;
    if (di->hash == hash
            && di->flags != CDI_FLAG_DEL
            && (d->usePkey
                 ? (di->Pkey != NULL && Pkey != NULL
                                               && strcmp(di->Pkey, Pkey) == 0)
                 : di->Ikey == Ikey))
      return di;
    if (di->flags == CDI_FLAG_DEL && freeitem == NULL)
      freeitem = di;
  }
}

void ZDictResize(CDictHead *d, int minitems) {
  CDictItem  temparray[HT_INIT_SIZE];
  CDictItem  *oldarray, *newarray;
  CDictItem  *olditem, *newitem;
  int        newi;
  int        todo;
  Zhashtype  oldsize, newsize;
  Zhashtype  minsize;
  Zhashtype  newmask;
  Zhashtype  perturb;

  if (d->locked > 0)
    return;

#ifdef DICT_DEBUG
  printf("size: %lu, filled: %lu, used: %lu\n",
           d->mask + 1, d->filled, d->used);
#endif

  if (minitems == 0) {
    if (d->filled < HT_INIT_SIZE - 1 && d->array == d->smallArray) {
#ifdef DICT_DEBUG
      printf("small array not full\n");
#endif
      return;
    }
    oldsize = d->mask + 1;
    if (d->filled * 3 < oldsize * 2 && d->used > oldsize / 5) {
#ifdef DICT_DEBUG
      printf("size OK\n");
#endif
      return;
    }
    if (d->used > 1000)
      minsize = d->used * 2;
    else
      minsize = d->used * 4;
  } else {
    if ((Zhashtype)minitems < d->used)
      minitems = (int)d->used;
    minsize = minitems * 3 / 2;
  }

  newsize = HT_INIT_SIZE;
  while (newsize < minsize) {
    newsize <<= 1;
    if (newsize == 0) {
      /* TODO: throw exception */
      fputs("EXCEPTION: ZDictResize\n", stdout);
      return;
    }
  }

#ifdef DICT_DEBUG
  printf("resizing from %lu to %lu\n", d->mask + 1, newsize);
#endif

  if (newsize == HT_INIT_SIZE) {
    newarray = d->smallArray;
    if (d->array == newarray) {
      memmove(temparray, newarray, sizeof(temparray));
      oldarray = temparray;
    } else
      oldarray = d->array;
    memset(newarray, 0, (size_t)(sizeof(CDictItem) * newsize));
  } else {
    newarray = (CDictItem *)Zalloc((sizeof(CDictItem) * newsize));
    if (newarray == NULL) {
      /* TODO: throw exception */
      fputs("EXCEPTION: ZDictResize\n", stdout);
      return;
    }
    oldarray = d->array;
  }

  newmask = newsize - 1;
  todo = (int)d->used;
  for (olditem = oldarray; todo > 0; ++olditem)
    if (olditem->flags & CDI_FLAG_USED) {
      newi = (int)(olditem->hash & newmask);
      newitem = &newarray[newi];
      if (newitem->flags != 0)
        for (perturb = olditem->hash; ; perturb >>= PERTURB_SHIFT) {
          newi = (int)((newi << 2) + newi + perturb + 1);
          newitem = &newarray[newi & newmask];
          if (newitem->flags == 0)
            break;
        }
      *newitem = *olditem;
      --todo;
    }

  if (d->array != d->smallArray)
      free(d->array);
  d->array = newarray;
  d->mask = newmask;
  d->filled = d->used;
}

/* "ow" is the overwrite flag.  When zero it's not allowed to overwrite an
existing entry. */
CDictHead *ZDictAdd(int ow, CDictHead *d, Zint Ikey, void *Pkey, Zint Ivalue, void *Pvalue) {
  Zhashtype  hash = ZDictHash(d, Ikey, Pkey);
  CDictItem  *di = ZDictLookup(d, Ikey, Pkey, hash);
#ifdef DICT_DEBUG
  if (d->usePkey)
    printf("Adding item %s\n", (char *)Pkey);
  else
    printf("Adding item %lld\n", Ikey);
  if (Pkey != NULL
      && (strcmp(Pkey, "ENUM") == 0
          || strcmp(Pkey, "EQUAL") == 0
          || strcmp(Pkey, "EXIT") == 0))
    dumpdict(d);
#endif
  if (di->flags == 0 || di->flags == CDI_FLAG_DEL || ow) {
    if (di->flags == 0 || di->flags == CDI_FLAG_DEL) {
      ++d->used;
      if (di->flags == 0)
        ++d->filled;
    }
    di->hash = hash;
    if (d->usePkey)
      di->Pkey = Pkey;
    else
      di->Ikey = Ikey;
    di->flags = CDI_FLAG_USED;

    if (Pvalue == NULL) {
      di->Ival = Ivalue;
      di->flags &= ~CDI_FLAG_PVAL;
    } else {
      di->Pval = Pvalue;
      di->flags |= CDI_FLAG_PVAL;
    }

    ZDictResize(d, 0);
  } else {
    /* TODO: throw exception? */
    if (d->usePkey)
      printf("EXCEPTION: ZDictAdd for %s\n", (char *)Pkey);
    else
      printf("EXCEPTION: ZDictAdd for %lld\n", Ikey);
  }
  return d;
}

#ifdef DICT_DEBUG
dumpdict(CDictHead *d)
{
  int        todo = (int)d->used;
  CDictItem  *item;
  int        idx = 0;

  for (item = d->array; todo > 0; ++item) {
    if (item->flags & CDI_FLAG_USED) {
      printf("%2d: %s\n", idx, (char *)item->Pkey);
      --todo;
    } else if (item->flags == 0) {
      printf("%2d: unused\n", idx);
    } else if (item->flags == CDI_FLAG_DEL) {
      printf("%2d: deleted\n", idx);
    } else {
      printf("%2d: invalid flags: %d\n", idx, item->flags);
    }
    ++idx;
  }
}
#endif


CDictItem *ZDictFind(CDictHead *d, Zint Ikey, void *Pkey) {
  Zhashtype  hash = ZDictHash(d, Ikey, Pkey);
  CDictItem  *di = ZDictLookup(d, Ikey, Pkey, hash);
  if (di->flags & CDI_FLAG_USED)
    return di;
  return NULL;
}

void *ZDictGetPtr(CDictHead *d, Zint Ikey, void *Pkey) {
  CDictItem *di = ZDictFind(d, Ikey, Pkey);
  if (di != NULL)
    return di->Pval;
  fputs("EXCEPTION: ZDictGetPtr\n", stdout);
  return NULL;  /* TODO: throw exception */
}

void *ZDictGetPtrDef(CDictHead *d, Zint Ikey, void *Pkey, void *def) {
  CDictItem *di = ZDictFind(d, Ikey, Pkey);
  if (di != NULL)
    return di->Pval;
  return def;
}

Zint ZDictGetInt(CDictHead *d, Zint Ikey, void *Pkey) {
  CDictItem *di = ZDictFind(d, Ikey, Pkey);
  if (di != NULL)
    return di->Ival;
  fputs("EXCEPTION: ZDictGetInt\n", stdout);
  return 0;  /* TODO: throw exception */
}

Zint ZDictGetIntDef(CDictHead *d, Zint Ikey, void *Pkey, Zint def) {
  CDictItem *di = ZDictFind(d, Ikey, Pkey);
  if (di != NULL)
    return di->Ival;
  return def;
}

Zbool ZDictHas(CDictHead *d, Zint Ikey, void *Pkey) {
  return (ZDictFind(d, Ikey, Pkey) != NULL);
}

CListHead *ZDictKeys(CDictHead *d) {
  CListHead *l = Zalloc(sizeof(CListHead));
  int hash;
  int first = 1;
  int todo;
  CDictItem *di;
  todo = d->used;
  for (di = d->array; todo > 0; ++di) {
    if (di->flags & CDI_FLAG_USED) {
      --todo;
      if (d->usePkey)
        ZListAdd(l, -1, 0, di->Pkey, 1);
      else
        ZListAdd(l, -1, di->Ikey, NULL, 0);
    }
  }
  return l;
}

Zfor_T *ZforNew(void *p, int type) {
  Zfor_T *s = Zalloc(sizeof(Zfor_T));
  s->type = type;
  if (type == 2)
    s->listItem = ((CListHead *)p)->first;
  return s;
}
void ZforGetPtr(Zfor_T *s, char **p) {
  if (s->type == 2 && s->listItem != NULL)
    *p = s->listItem->Pval;
  else
    *p = NULL;
}
void ZforGetInt(Zfor_T *s, Zint *p) {
  if (s->type == 2 && s->listItem != NULL)
    *p = s->listItem->Ival;
  else
    *p = 0;
}
int ZforCont(Zfor_T *s) {
  if (s->type == 2)
    return s->listItem != NULL;
  return 0;
}
void ZforNextPtr(Zfor_T *s, char **p) {
  if (s->type == 2 && s->listItem != NULL)
    s->listItem = s->listItem->next;
  ZforGetPtr(s, p);
}
void ZforNextInt(Zfor_T *s, Zint *p) {
  if (s->type == 2 && s->listItem != NULL)
    s->listItem = s->listItem->next;
  ZforGetInt(s, p);
}

MIO__CStat *ZStat(char *name) {
  MIO__CStat *res = Zalloc(sizeof(MIO__CStat));
  struct stat st;
  if (stat(name, &st) == 0) {
    res->size = st.st_size;
    res->time = st.st_mtime;
  }
  return res;
}
#ifndef INC_builtin_D
#include "ZUDIR/builtin.d.h"
#endif
#ifndef INC_config_D
#include "ZUDIR/config.d.h"
#endif
#ifndef INC_error_D
#include "ZUDIR/error.d.h"
#endif
#ifndef INC_generate_D
#include "ZUDIR/generate.d.h"
#endif
#ifndef INC_node_D
#include "ZUDIR/node.d.h"
#endif
#ifndef INC_output_D
#include "ZUDIR/output.d.h"
#endif
#ifndef INC_parse_D
#include "ZUDIR/parse.d.h"
#endif
#ifndef INC_scope_D
#include "ZUDIR/scope.d.h"
#endif
#ifndef INC_usedfile_D
#include "ZUDIR/usedfile.d.h"
#endif
#ifndef INC_write_c_D
#include "ZUDIR/write_c.d.h"
#endif
void ZglobInit();
/*
 * FUNCTION BODIES
 */
#ifndef INC_builtin_B
#include "ZUDIR/builtin.b.c"
#endif
#ifndef INC_config_B
#include "ZUDIR/config.b.c"
#endif
#ifndef INC_error_B
#include "ZUDIR/error.b.c"
#endif
#ifndef INC_generate_B
#include "ZUDIR/generate.b.c"
#endif
#ifndef INC_node_B
#include "ZUDIR/node.b.c"
#endif
#ifndef INC_output_B
#include "ZUDIR/output.b.c"
#endif
#ifndef INC_parse_B
#include "ZUDIR/parse.b.c"
#endif
#ifndef INC_scope_B
#include "ZUDIR/scope.b.c"
#endif
#ifndef INC_usedfile_B
#include "ZUDIR/usedfile.b.c"
#endif
#ifndef INC_write_c_B
#include "ZUDIR/write_c.b.c"
#endif
/*
 * INIT GLOBALS
 */
void ZglobInit() {
  Ibuiltin();
  Iconfig();
  Ierror();
  Igenerate();
  Inode();
  Ioutput();
  Iparse();
  Iscope();
  Iusedfile();
  Iwrite_c();
}

/*
 * MAIN
 */
int main(int argc, char **argv) {
  MARG__Vcount = argc - 1;
  MARG__Vname = argv[0];
  MARG__Vargs = argv + 1;
  ZglobInit();
return Fmain();
}
int Fmain() {
  char *VinFileName;
  VinFileName = NULL;
  char *VprogName;
  VprogName = NULL;
  Zbool Vexecute;
  Vexecute = 0;
  char *VexecuteArgs;
  VexecuteArgs = "";
  if ((MARG__Vcount == 0))
  {
    fputs(Zconcat(Zconcat("Usage: ", MARG__Vname), " [-x] [-d] [-v] [-cc compiler] [-o progname] source.zu ...\n"), stdout);
    exit(1);
  }
  Zint Vidx;
  Vidx = 0;
  while (1)
  {
    if ((Zstrcmp(MARG__Vargs[Vidx], "-x") == 0))
    {
      Vexecute = 1;
      ++(Vidx);
      continue;
    }
    if ((Zstrcmp(MARG__Vargs[Vidx], "-d") == 0))
    {
      MError__Vdebug = 1;
      ++(Vidx);
      continue;
    }
    if ((Zstrcmp(MARG__Vargs[Vidx], "-v") == 0))
    {
      ++(MError__Vverbose);
      ++(Vidx);
      continue;
    }
    if (((Zstrcmp(MARG__Vargs[Vidx], "-o") == 0) && ((MARG__Vcount - Vidx) > 2)))
    {
      VprogName = MARG__Vargs[(Vidx + 1)];
      Vidx += 2;
      continue;
    }
    if (((Zstrcmp(MARG__Vargs[Vidx], "-cc") == 0) && ((MARG__Vcount - Vidx) > 2)))
    {
      MConfig__Vcompiler = MARG__Vargs[(Vidx + 1)];
      Vidx += 2;
      continue;
    }
    break;
  }
  if (((MARG__Vcount - Vidx) < 1))
  {
    fputs("Missing source file argument\n", stdout);
    exit(1);
  }
  VinFileName = MARG__Vargs[Vidx];
  ++(Vidx);
  if ((Vidx < MARG__Vcount))
  {
    if (!(Vexecute))
    {
      fputs("SORRY, only one .zu file is currently allowed\n", stdout);
      exit(1);
    }
    do
    {
      VexecuteArgs = Zconcat(VexecuteArgs, Zconcat(" ", MARG__Vargs[Vidx]));
      ++(Vidx);
    }
      while (!((Vidx == MARG__Vcount)));
  }
  if ((Zstrcmp(ZStringByteSlice(VinFileName, -(3), -(1)), ".zu") != 0))
  {
    (fputs(Zconcat("ERROR: Input name must end in '.zu': ", VinFileName), stdout) | fputc('\n', stdout));
    exit(1);
  }
  if ((MConfig__Frun() != 1))
  {
    fputs("ERROR: Config failed, cannot compile a program\n", stdout);
    exit(1);
  }
  char *VrootName;
  VrootName = ZStringByteSlice(VinFileName, 0, -(4));
  if ((VprogName == NULL))
  {
    VprogName = Zconcat(VrootName, MConfig__VexeSuffix);
  }
  CBuiltin__X__Fprepare();
  char *VoutFileName;
  VoutFileName = Zconcat(VrootName, ".c");
  FILE *VoutFile;
  VoutFile = fopen(VoutFileName, "w");
  if ((VoutFile == NULL))
  {
    (fputs(Zconcat("ERROR: Cannot open file for writing: ", VoutFileName), stdout) | fputc('\n', stdout));
    exit(1);
  }
  CUsedFile *VmainFile;
  VmainFile = CUsedFile__FNEW(VinFileName, (1 + 2));
  if ((CUsedFile__Fparse(VmainFile, "") == 0))
  {
    (fputs(Zconcat("ERROR: Cannot open file for reading: ", VinFileName), stdout) | fputc('\n', stdout));
    exit(1);
  }
  Zbool VdoPass;
  VdoPass = 1;
  while ((VdoPass && !(MError__VfoundError)))
  {
    VdoPass = MGenerate__Fresolve(VmainFile);
  }
  if (!(MError__VfoundError))
  {
    COutput__X__CHeads *Vheads;
    Vheads = COutput__X__CHeads__FNEW__1();
    COutput__X__CGroup *Voutputs;
    Voutputs = COutput__X__CGroup__FNEW__2(Vheads);
    MGenerate__Fwrite(VmainFile, Voutputs);
    fputs("/*\n", VoutFile);
    fputs(Zconcat(Zconcat(" * Generated from Zimbu file ", VinFileName), "\n"), VoutFile);
    fputs(" */\n", VoutFile);
    CWrite_C__X__FwriteFile(Vheads, CUsedFile__Fscope(VmainFile), VoutFile);
  }
  fclose(VoutFile);
  Zint Vretval;
  Vretval = 1;
  if (!(MError__VfoundError))
  {
    unlink(VprogName);
    MIO__CStat *Vbefore;
    Vbefore = ZStat(VprogName);
    char *Vcmd;
    Vcmd = MConfig__FcompilerCmd(VoutFileName, VprogName);
    if ((MError__Vverbose > 0))
    {
      (fputs(Zconcat("Executing compiler: ", Vcmd), stdout) | fputc('\n', stdout));
    }
    Vretval = fflush(stdout), system(Vcmd);
    MIO__CStat *Vafter;
    Vafter = ZStat(VprogName);
    if ((Vafter->size == 0))
    {
      fputs("ERROR: Compiled program is zero size\n", stdout);
      Vretval = 1;
    }
  else if (((Vbefore->size == Vafter->size) && (Vbefore->time == Vafter->time)))
    {
      fputs("ERROR: Compiled program was not updated\n", stdout);
      Vretval = 1;
    }
  }
  if (((Vretval == 0) && Vexecute))
  {
    if ((VprogName[0] != 47))
    {
      VprogName = Zconcat("./", VprogName);
    }
    Vretval = fflush(stdout), system(Zconcat(VprogName, VexecuteArgs));
  }
  return Vretval;
}
