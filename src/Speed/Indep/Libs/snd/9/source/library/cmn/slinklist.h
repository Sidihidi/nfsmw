#ifndef SLINKLIST_H
#define SLINKLIST_H

#include <cstddef>

// total size: 0x4
typedef struct CListNode {
private:
    CListNode *pnext; // offset 0x0, size 0x4

public:
    inline void SetNext(CListNode *pnode) {
        this->pnext = pnode;
    }

    inline CListNode *GetNext() {
        return this->pnext;
    }
} CListNode;

// total size: 0x4
class CListStack {
private:
    CListNode *phead; // offset 0x0, size 0x4

public:
    CListStack() : phead(NULL) {}

    void Reset() {
        this->phead = NULL;
    }

    bool IsEmpty() {
        return this->phead == NULL;
    }

    CListNode *GetHead() { return phead; }

    void Push(CListNode *pnode) {
        pnode->SetNext(this->phead);
        this->phead = pnode;
    }

    CListNode *Pop() {
        CListNode *pnode = this->phead;

        if (pnode != NULL) {
            this->phead = pnode->GetNext();
        }

        return pnode;
    }
};

// total size: 0x8
// Decl: 24
typedef struct CListDNode {
private:
    CListDNode *pnext; // offset 0x0, size 0x4
    CListDNode *pprev; // offset 0x4, size 0x4

public:
    inline void SetNext(CListDNode *pnode) {
        this->pnext = pnode;
    }

    inline void SetPrev(CListDNode *pnode) {
        this->pprev = pnode;
    }

    inline CListDNode *GetNext() {
        return this->pnext;
    }

    inline CListDNode *GetPrev() {
        return this->pprev;
    }
} CListDNode;

// total size: 0x4
// Decl: 96
class CListDStack {
private:
    CListDNode *phead; // offset 0x0, size 0x4, Decl: 101

public:
    CListDStack() : phead(NULL) {} // Decl: 104

    void Reset() {
        this->phead = NULL;
    } // Decl: 106

    bool IsEmpty() {
        return this->phead == NULL;
    } // Decl: 108

    CListDNode *GetHead() { return phead; } // Decl: 110

    void Push(CListDNode *pnode) {
        pnode->SetNext(phead);
        pnode->SetPrev(NULL);
        if (this->phead != NULL) {
            this->phead->SetPrev(pnode);
        }
        this->phead = pnode;
    } // Decl: 112

    // r76-audit: Pop venia decompilado con cuerpo {} (caer por el final de una
    // funcion no-void es UB; GCC 2.95 lo tolera, clang -O2 no genera nada
    // despues). El DWARF del binario original registra la instancia abstracta
    // de este inline con UN local (CListDNode *pnode), asi que el fuente
    // original llevaba cuerpo. Nadie lo llama en todo el arbol y no se emite
    // (el objeto original no tiene simbolo Pop), con lo que la forma exacta no
    // es recuperable: se restaura la simetrica de CListStack::Pop. Objeto
    // invariante (rebuild + objdiff verificados).
    CListDNode *Pop() {
        CListDNode *pnode = this->phead;

        if (pnode != NULL) {
            this->phead = pnode->GetNext();
        }

        return pnode;
    } // Decl: 129

    void Remove(CListDNode *pnode) {
        if (pnode == this->phead) {
            this->phead = pnode->GetNext();
        }

        if (pnode->GetPrev() != NULL) {
            CListDNode *ptempnode = pnode->GetPrev();
            ptempnode->SetNext(pnode->GetNext());
        }

        if (pnode->GetNext() != NULL) {
            CListDNode *ptempnode = pnode->GetNext();
            ptempnode->SetPrev(pnode->GetPrev());
        }
    } // Decl: 148
};

// total size: 0xC
class CListQueue {
private:
    CListNode *phead;  // offset 0x0, size 0x4
    CListNode *ptail;  // offset 0x4, size 0x4
    int entries;       // offset 0x8, size 0x4

public:
    CListQueue() : phead(NULL), ptail(NULL), entries(0) {}

    void Reset() {
        this->phead = NULL;
        this->ptail = NULL;
        this->entries = 0;
    }

    bool IsEmpty() {
        return this->phead == NULL;
    }

    CListNode *GetHead() { return phead; }

    CListNode *GetTail() { return ptail; }

    int GetEntries() { return entries; }

    void Push(CListNode *pnode) {
        pnode->SetNext(this->phead);
        this->phead = pnode;
        if (this->ptail == NULL) {
            this->ptail = pnode;
        }
        this->entries++;
    }

    void PushTail(CListNode *pnode) {
        pnode->SetNext(NULL);
        if (this->phead == NULL) {
            this->phead = pnode;
        } else {
            this->ptail->SetNext(pnode);
        }
        this->ptail = pnode;
        this->entries++;
    }

    CListNode *Pop() {
        CListNode *pnode = this->phead;

        if (pnode != NULL) {
            this->phead = pnode->GetNext();
            if (this->phead == NULL) {
                this->ptail = NULL;
            }
            this->entries--;
        }

        return pnode;
    }
};

#endif
