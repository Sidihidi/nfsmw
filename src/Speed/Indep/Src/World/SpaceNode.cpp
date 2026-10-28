#include "SpaceNode.hpp"

#include "Speed/Indep/bWare/Inc/bMath.hpp"
#include "Speed/Indep/bWare/Inc/bMemory.hpp"
#include "Speed/Indep/bWare/Inc/bSlotPool.hpp"

#ifdef EA_PLATFORM_PLAYSTATION2
// gcdatos21: en las dos PS2 el orden es [SpaceNodeTrashList]
// [SpaceNodeSlotPool] (PS2_globals:5936-5937 y el .data objetivo de la SLUS,
// +0x12c4/+0x12cc). SpaceNodeSlotPool es POD; la TrashList conserva su orden
// relativo de ctor.
bTList<SpaceNode> SpaceNodeTrashList;

SlotPool *SpaceNodeSlotPool = nullptr;
#else
SlotPool *SpaceNodeSlotPool = nullptr;

bTList<SpaceNode> SpaceNodeTrashList;
#endif

SpaceNode::SpaceNode(SpaceNode *parent) {
    this->ReferenceCount = 0;
    this->Dirty = 1;

    bIdentity(&this->LocalMatrix);

    this->LocalVelocity = bVector3(0.0f, 0.0f, 0.0f);
    this->LocalAngularVelocity = bVector3(0.0f, 0.0f, 0.0f);
    this->WorldAngularVelocity = bVector3(0.0f, 0.0f, 0.0f);

    this->Parent = nullptr;
    this->SetParent(parent);

    this->BlendingMatrices = nullptr;
}

SpaceNode::~SpaceNode() {
    this->RemoveAllChildren();
    this->RemoveFromParent();
}

// STRIPPED
void SpaceNode::SetName(const char *name) {
    this->Name = name;
}

void SpaceNode::SetParent(SpaceNode *new_parent) {
    SpaceNode *current_parent = this->GetParent();
    if (new_parent == current_parent) {
        return;
    }
    this->RemoveFromParent();
    if (new_parent != nullptr) {
        new_parent->AddChild(this);
    }
}


void SpaceNode::RemoveFromParent() {
    SpaceNode *parent = this->GetParent();
    if (parent != nullptr) {
        parent->RemoveChild(this);
    }
}

void SpaceNode::AddChild(SpaceNode *child) {
    child = this->ChildrenList.AddTail(child);
    child->Parent = this;
    this->Lock();
}

void SpaceNode::RemoveChild(SpaceNode *child) {
    child = child->Remove();
    child->Parent = nullptr;
    this->Unlock();
}

void SpaceNode::RemoveAllChildren() {
    while (!this->ChildrenList.IsEmpty()) {
        this->RemoveChild(this->ChildrenList.GetHead());
    }
}

void SpaceNode::Lock() {
    this->ReferenceCount++;
}

void SpaceNode::Unlock() {
    this->ReferenceCount--;
    if (this->ReferenceCount == 0) {
        this->RemoveFromParent();
        SpaceNodeTrashList.AddTail(this);
    }
}

void SpaceNode::ReallySetDirty() {
    this->Dirty = 1;

    for (SpaceNode *node = this->ChildrenList.GetHead(); node != this->ChildrenList.EndOfList(); node = node->GetNext()) {
        node->SetDirty();
    }
}

void SpaceNode::Update() {
    if (this->Parent != nullptr) {
        bMulMatrix(&this->WorldMatrix, this->Parent->GetWorldMatrix(), &this->LocalMatrix);
        bVector3 rotated_velocity;
        bMulMatrix(&rotated_velocity, this->Parent->GetWorldMatrix(), &this->LocalVelocity);
        rotated_velocity -= *reinterpret_cast<bVector3 *>(&this->Parent->GetWorldMatrix()->v3);
        this->WorldVelocity = *this->Parent->GetWorldVelocity() + rotated_velocity;
    } else {
        bCopy(&this->WorldMatrix, &this->LocalMatrix);
        bCopy(&this->WorldVelocity, &this->LocalVelocity);
    }

    Dirty = 0;
}

// STRIPPED
void SpaceNode::SetWorldMatrix(bMatrix4 *matrix) {}

// STRIPPED
void SpaceNode::SetWorldVelocity(bVector3 *velocity) {}

SpaceNode *CreateSpaceNode(SpaceNode *parent) {
    SpaceNode *sn = new SpaceNode(parent);

    sn->Lock();
    return sn;
}

void DeleteSpaceNode(SpaceNode *space_node) {
    space_node->Unlock();
}

// STRIPPED
SpaceNode *FindSpaceNode(uint32 name_hash, SpaceNode *parent) {}

void ServiceSpaceNodes() {
    while (!SpaceNodeTrashList.IsEmpty()) {
        SpaceNode *head = SpaceNodeTrashList.RemoveHead();

        delete head;
    }
}

void InitSpaceNodes() {
    SpaceNodeSlotPool = bNewSlotPool(sizeof(SpaceNode), 80, "SpaceNodeSlotPool", GetVirtualMemoryAllocParams());
}

// STRIPPED
void CloseSpaceNodes() {}
