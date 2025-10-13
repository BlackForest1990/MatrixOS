// kernel/include/list.h
#ifndef LIST_H
#define LIST_H

struct list_head {
    struct list_head *next, *prev;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }
#define LIST_HEAD(name) struct list_head name = LIST_HEAD_INIT(name)

static inline void list_init(struct list_head *head) {
    head->next = head;
    head->prev = head;
}

static inline void list_add(struct list_head *new, struct list_head *head) {
    new->next = head->next;
    new->prev = head;
    head->next->prev = new;
    head->next = new;
}

static inline void list_del(struct list_head *entry) {
    entry->next->prev = entry->prev;
    entry->prev->next = entry->next;
    entry->next = entry->prev = NULL;
}

static inline int list_empty(struct list_head *head) {
    return head->next == head;
}

#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#endif
