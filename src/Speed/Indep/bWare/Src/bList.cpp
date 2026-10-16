#include "Speed/Indep/bWare/Inc/bList.hpp"
#include "Speed/Indep/bWare/Inc/bMemory.hpp"
#include "Speed/Indep/bWare/Inc/bSlotPool.hpp"
#include "Speed/Indep/Libs/Support/Utility/UVectorMath.h"

bNode *bList::GetNode(int ordinal_number) {
    int n = 0;
    bNode *node = this->GetHead();
    while (node != this->EndOfList() && (n != ordinal_number)) {
        n++;
        node = node->GetNext();
    }
    return node;
}

int bList::TraversebList(bNode *match_node) {
    bNode *node = this->GetHead();
    int n = 0;
    while (node != this->EndOfList()) {
        if (node == match_node) {
            return n + 1;
        }
        n++;
        node = node->GetNext();
    }
    if (match_node) {
        n = 0;
    }
    return n;
}

// TODO
void bList::AddTail(bList *list) {}

// TODO
void bList::AddHead(bList *list) {}

// STRIPPED
bNode *bList::AddSorted(SortFunc check_flip, bNode *node) {}

// La pasada va dentro de un while (true) que acaba en break, con if/else. Lo dicen tres
// cosas del original: los stabs de la A124 (el bloque del bucle envuelve todo el cuerpo y el
// if tiene bloque then y bloque else), la A124 llama a MergeSort con jal y no con j (una
// llamada dentro de un bucle no puede ser de cola) y las dos notas de linea del epilogo de
// GameCube (el break y el cierre del bucle). Locales en el orden del DWARF.
void bList::Sort(SortFunc check_flip) {
    bNode *node;
    bNode *next_node;
    int did_swap = 0;

    while (true) {
        node = this->GetHead();
        next_node = node->GetNext();
        while (node != this->EndOfList() && next_node != this->EndOfList()) {
            if (check_flip(node, next_node) == 0) {
                next_node->Remove();
                next_node->AddBefore(node);
                did_swap++;
                next_node = node->GetNext();
            } else {
                node = next_node;
                next_node = next_node->GetNext();
            }
        }
        if (did_swap != 0) {
            this->MergeSort(check_flip);
        }
        break;
    }
}

void bList::MergeSort(SortFunc cmp) {
    if (IsEmpty()) {
        return;
    }

    bNode *list = GetHead();
    list->Prev = GetTail();
    GetTail()->Next = list;

    bNode *p;
    bNode *q;
    bNode *e;
    bNode *tail;
    bNode *oldhead;
    int insize = 1;
    int nmerges;
    int psize;
    int qsize;
    int i;

    while (true) {
        p = list;
        oldhead = p;
        list = nullptr;
        tail = nullptr;
        nmerges = 0;

        while (p) {
            nmerges++;

            q = p;
            psize = 0;
            for (i = 0; i < insize; i++) {
                psize++;
                q = q->Next == oldhead ? nullptr : q->Next;
                if (q == nullptr)
                    break;
            }
            qsize = insize;
            while (psize > 0 || (qsize > 0 && q)) {
                if (psize == 0) {
                    e = q;
                    q = q->Next;
                    qsize--;
                    if (q == oldhead) {
                        q = nullptr;
                    }
                } else if (qsize == 0 || !q) {
                    e = p;
                    p = p->Next;
                    psize--;
                    if (p == oldhead) {
                        p = nullptr;
                    }
                } else if (cmp(p, q) > 0) {
                    psize--;
                    e = p;
                    p = p->Next;
                    if (p == oldhead) {
                        p = nullptr;
                    }
                } else {
                    qsize--;
                    e = q;
                    q = q->Next;
                    if (q == oldhead) {
                        q = nullptr;
                    }
                }

                if (tail) {
                    tail->Next = e;
                } else {
                    list = e;
                }

                e->Prev = tail;
                tail = e;
            }
            p = q;
        }

        tail->Next = list;
        list->Prev = tail;
        if (nmerges <= 1) {
            list->Prev = &this->HeadNode;
            this->HeadNode.Next = list;
            this->HeadNode.Prev = tail;
            tail->Next = &this->HeadNode;
            return;
        }

        insize *= 2;
    }
}

SlotPool *bPNodeSlotPool = nullptr;
int bPListWantToClose = false;
int bPListAllocationNumber = 0;

void bPListInit(int num_expected_bpnodes) {
    if (!bPNodeSlotPool) {
        bPNodeSlotPool = bNewSlotPool(12, num_expected_bpnodes, "bPNode SlotPool", GetVirtualMemoryAllocParams());
        bPListWantToClose = false;
    }
}

void bPListClose() {
    if (bPNodeSlotPool) {
        if (bPNodeSlotPool->IsEmpty()) {
            bDeleteSlotPool(bPNodeSlotPool);
            bPNodeSlotPool = nullptr;
        } else {
            bPListWantToClose = true;
        }
    }
}

void *bPNode::Malloc() {
    if (!bPNodeSlotPool) {
        bPListInit(256);
    }
    return bOMalloc(bPNodeSlotPool);
}

void bPNode::Free(void *ptr) {
    bFree(bPNodeSlotPool, ptr);
    if (bPListWantToClose) {
        bPListClose();
    }
}
