#include "Speed/Indep/bWare/Inc/bMemory.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/bWare/Inc/bPrintf.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"

#include <cstring>
#include <types.h>

#ifdef EA_PLATFORM_PLAYSTATION2
#include "Speed/Indep/bWare/Inc/bFunk.hpp"
#include <stdlib.h>
#endif

#ifdef EA_PLATFORM_XENON
#include <xtl.h>
#endif

#ifdef EA_PLATFORM_GAMECUBE
#include "dolphin/os/OSArena.h"
#ifdef EA_PLATFORM_GAMECUBE
#include <dolphin.h>
#endif
#endif

// TODO
extern "C" {
void VMInit(size_t, uintptr_t, size_t);
BOOL VMAlloc(uintptr_t, size_t);
}

#ifdef MILESTONE_BUILD
// total size: 0x8. Va delante de la AllocationHeader en las milestone (stabs de la A124).
struct DebugAllocationHeader {
    static int GetRequiredSize(const char *debug_text, int shared_string_index) {
        if (shared_string_index == -1) {
            return (bStrLen(debug_text) + 10) & ~3U; // 6 B de cabecera, la cadena con su 0, a 4 B
        }
        return 8;
    }

    uint16 AllocationNumber; // offset 0x0, size 0x2
    uint16 DebugLine;        // offset 0x2, size 0x2
    int16 SharedStringIndex; // offset 0x4, size 0x2
    char DebugText[2];       // offset 0x6, size 0x2
};
#endif

// total size: 0x14
class AllocationHeader : public bTNode<AllocationHeader> {
  public:
    void *GetBottomAddress() {
        return reinterpret_cast<void *>(reinterpret_cast<char *>(this) - FrontPadding);
    }

    void *GetAllocAddress() {
        return &this[1];
    }

#ifdef MILESTONE_BUILD
    DebugAllocationHeader *GetDebugHeader() {
        return reinterpret_cast<DebugAllocationHeader *>(GetBottomAddress());
    }

    const char *GetDebugText() {
        const char *s = bGetSharedString(GetDebugHeader()->SharedStringIndex);
        return s ? s : GetDebugHeader()->DebugText;
    }
#else
    const char *GetDebugText() {
        return "";
    }
#endif

    int GetAllocationNumber() {
#ifdef MILESTONE_BUILD
        return *reinterpret_cast<uint16 *>(reinterpret_cast<char *>(this) - FrontPadding);
#else
        return 0;
#endif
    }

    int GetDebugLine() {
#ifdef MILESTONE_BUILD
        return *reinterpret_cast<uint16 *>(reinterpret_cast<char *>(this) - FrontPadding + 2);
#else
        return 0;
#endif
    }

    uint8 PoolNum;       // offset 0x8, size 0x1
    uint8 MagicNumber;   // offset 0x9, size 0x1
    uint16 FrontPadding; // offset 0xA, size 0x2
    int32 Size;          // offset 0xC, size 0x4
    int32 RequestedSize; // offset 0x10, size 0x4
};

#ifdef EA_PLATFORM_WIN32
// Solo PC: el exe de PC tiene una tercera lista en la MemoryPool (0x14): Init la inicializa, Close la vacia con delete
// y AddFreeMemory/AllocateMemory la usan con EnableCleanupBorrowedMemoryBlock y BorrowMemoryBlockMinSize. Xbox 360,
// GameCube y las PS2 no la tienen. PC no trae simbolos: el nombre del tipo y el del miembro son nuestros.
class BorrowedMemoryBlock : public bTNode<BorrowedMemoryBlock> {
  public:
    void *Memory; // offset 0x8, size 0x4
    int Size;     // offset 0xC, size 0x4
};
#endif

// total size: 0x60
class MemoryPool {
  public:
    void Init(void *memory, int memory_size, const char *debug_name);
    void Close();
    void AddMemory(void *p, int size);
    void RemoveMemory(void *p, int size);
    void FreeMemory(void *p, int size
#ifndef EA_BUILD_A124 // la alpha 124 no pasa el nombre de depuracion
                    ,
                    const char *debug_name
#endif
    );
    void AddFreeMemory(void *p, int size
#ifndef EA_BUILD_A124
                       ,
                       const char *debug_name
#endif
    );
    void *AllocateMemory(int size, int alignment, int alignment_offset, int start_from_top, int use_best_fit, int *new_size);
    int GetAmountFree();
    int GetLargestFreeBlock();
    void VerifyPoolIntegrity(bool verify_free_pattern);
    int CountAllocations(const char *debug_text);
    void PrintAllocationsByAddress(int from_allocation, int to_allocation);
    void PrintAllocations(int from_allocation, int to_allocation);
    AllocationHeader *FindAllocation(int allocation_num);
    int GetAllocations(void **allocations, int max_allocations);
#ifdef EA_BUILD_A124
    void *GetMemoryDumpStatistics(int &num_allocs, int &high_allocs, int &usage, int &high_usage, int &free, const char *&name);
#endif
    void SetFancyStompDetector(void *mem, int mem_size, const char *name);
    bool CheckFancyStompDetector(const void *mem, int mem_size);
    void TraceNewPool();
    void TraceDeletePool();
    void TraceFreeMemory(void *p, int size);
    void TraceRemoveMemory(void *p, int size);
    void TraceAllocateMemory(void *p, int size);
    void UpdateTraceInformation();

    const char *GetName() {
        return this->pDebugName;
    }

    bool SetDebugFill(bool on_off) {
        bool previous = this->DebugFillEnabled;
        this->DebugFillEnabled = on_off;
        return previous;
    }

    bool SetDebugTracing(bool on_off) {
        bool previous = this->DebugTracingEnabled;
        this->DebugTracingEnabled = on_off;
        return previous;
    }

    bool IsInPool(intptr_t address) {
        return address >= this->InitialAddress && address < this->InitialAddress + this->InitialSize;
    }

    int GetPoolSize() {
        return PoolSize;
    }

    bool IsEmpty() {}

    void AddAllocationHeader(AllocationHeader *allocation_header) {
        this->AllocationHeaderList.AddTail(allocation_header);
    }

    void RemoveAllocationHeader(AllocationHeader *allocation_header) {
        this->AllocationHeaderList.Remove(allocation_header);
    }

  private:
    const char *pDebugName;                        // offset 0x0, size 0x4
    bTList<FreeBlock> FreeBlockList;               // offset 0x4, size 0x8
    bTList<AllocationHeader> AllocationHeaderList; // offset 0xC, size 0x8
#ifdef EA_PLATFORM_WIN32
    bTList<BorrowedMemoryBlock> BorrowedMemoryBlockList; // offset 0x14, size 0x8 (solo PC)
#endif
    intptr_t InitialAddress;                       // offset 0x14, size 0x4
    int InitialSize;                               // offset 0x18, size 0x4
    int NumAllocations;                            // offset 0x1C, size 0x4
    int TotalNumAllocations;                       // offset 0x20, size 0x4
    int PoolSize;                                  // offset 0x24, size 0x4
    int AmountAllocated;                           // offset 0x28, size 0x4
    int MostAmountAllocated;                       // offset 0x2C, size 0x4
    int AmountFree;                                // offset 0x30, size 0x4
    int LeastAmountFree;                           // offset 0x34, size 0x4
    bool DebugFillEnabled;                         // offset 0x38, size 0x1
    bool DebugTracingEnabled;                      // offset 0x3C, size 0x1
    bMutex Mutex;                                  // offset 0x40, size 0x20
};

int bMemoryAutomaticVerifyPoolIntegrity = 0; // size: 0x4, address: 0x80416418
int bMemoryPrintEachAllocation = 0;
unsigned int bMemoryPrintAllocationRangeLow = 0;
unsigned int bMemoryPrintAllocationRangeHigh = 0;
int EnableCleanupBorrowedMemoryBlock = 1;
int BorrowMemoryBlockMinSize = 0x19000;
int bMemoryRandomFillPattern = 0; // size: 0x4, address: 0x80416430
int bMemoryUseSharedStrings = 1;
int bMemoryTracing = 0;                                                               // size: 0x4, address: 0x80416438
#ifdef MILESTONE_BUILD
typedef void (*bMemoryDebugCallback)(int pool_num, void *ptr, int size, const char *debug_text);
static bMemoryDebugCallback bMemoryAllocCallback = nullptr;
static bMemoryDebugCallback bMemoryFreeCallback = nullptr;
#endif
int bMemoryBreakOnAllocationNumber = -1;                                              // size: 0x4, address: 0x8041643C
int bMemoryAllocationNumber = 0;                                                      // size: 0x4, address: 0x80416440
bMemoryAllocator TheMemoryAllocator;                                                  // size: 0xC, address: 0x8045790C
EA::Allocator::IAllocator &gMemoryAllocator = TheMemoryAllocator;                     // size: 0x4, address: 0x80457918
bMemoryAllocator TheMemoryPersistentAllocator;                                        // size: 0xC, address: 0x8045791C
EA::Allocator::IAllocator &gMemoryPersistentAllocator = TheMemoryPersistentAllocator; // size: 0x4, address: 0x80457928
// static const int bMemoryEnableFancyStompDetector; // size: 0x4
#if defined(MILESTONE_BUILD) && !defined(EA_BUILD_A124)
int bMemoryBreakOnFancyStompDetection = 0; // DWARF de GameCube (sin direccion alli); X360 0x82A80E14
#endif

#ifdef EA_PLATFORM_GAMECUBE
void bFunkGameCube(const char *server_name, unsigned char function_num, const void *param_buffer, long param_size) {}
#endif

// STRIPPED
bool ShouldPrintThisAllocation(void *mem, const char *debug_name) {}

int GetAlignmentAdjustTop(intptr_t address, int alignment, int alignment_offset) {
    return (alignment - (address + alignment_offset & alignment - 1)) % alignment;
}

int GetAlignmentAdjustBottom(intptr_t address, int alignment, int alignment_offset) {
    return (address + alignment_offset & alignment - 1) % alignment;
}

void MemoryPool::Init(void *memory, int memory_size, const char *debug_name) {
    this->FreeBlockList.InitList();
    this->AllocationHeaderList.InitList();
#ifdef EA_PLATFORM_WIN32
    this->BorrowedMemoryBlockList.InitList();
#endif
    this->InitialAddress = reinterpret_cast<uintptr_t>(memory);
    this->InitialSize = memory_size;
    this->NumAllocations = 0;
    this->TotalNumAllocations = 0;
    this->PoolSize = 0;
    this->AmountAllocated = 0;
    this->MostAmountAllocated = 0;
    this->AmountFree = 0;
    this->LeastAmountFree = 0;
    this->Mutex.Create();
    this->DebugFillEnabled = true;
    this->DebugTracingEnabled = true;
    this->pDebugName = debug_name;
    if (bMemoryTracing) {
        this->TraceNewPool();
    }
    this->AddMemory(memory, memory_size);
}

void MemoryPool::Close() {
    if (this->NumAllocations > 0) {
        this->PrintAllocationsByAddress(0, 0x7fffffff);
        bBreak();
    }
    this->TraceDeletePool();
    this->Mutex.Destroy();
}

void MemoryPool::AddMemory(void *p, int size) {
    this->PoolSize += size;
    this->AddFreeMemory(p, size
#ifndef EA_BUILD_A124
                        ,
                        this->GetName()
#endif
    );
}

// STRIPPED
void MemoryPool::RemoveMemory(void *p, int size) {}

void MemoryPool::FreeMemory(void *p, int size
#ifndef EA_BUILD_A124
                            ,
                            const char *debug_name
#endif
) {
    this->Mutex.Lock();
    this->AddFreeMemory(p, size
#ifndef EA_BUILD_A124
                        ,
                        debug_name
#endif
    );
    this->AmountAllocated -= size;
    this->NumAllocations--;
    this->AmountFree = this->PoolSize - this->AmountAllocated;
    this->Mutex.Unlock();
}

void MemoryPool::AddFreeMemory(void *p, int size
#ifndef EA_BUILD_A124
                               ,
                               const char *debug_name
#endif
) {
    if (size != 0) {
        this->Mutex.Lock();

        if (bMemoryTracing && this->DebugTracingEnabled) {
            TraceFreeMemory(p, size);
        }

        FreeBlock *new_free_block = static_cast<FreeBlock *>(p);
        new_free_block->Size = size;
        new_free_block->MagicNumber = 0x44443333;

        FreeBlock *f = this->FreeBlockList.GetHead();
        while (f != this->FreeBlockList.EndOfList() && f <= new_free_block) {
            f = f->GetNext();
        }

        this->FreeBlockList.AddBefore(f, new_free_block);

        // a GetTop inline missing
        char *fill_bot = reinterpret_cast<char *>(new_free_block + 1);
        char *fill_top = reinterpret_cast<char *>(new_free_block->GetTop());
        if (fill_top == reinterpret_cast<char *>(f)) {
            fill_top = reinterpret_cast<char *>(f + 1);
            new_free_block->Size += f->Size;
            this->FreeBlockList.Remove(f);
        }

        f = new_free_block->GetPrev();
        if (f->GetTop() == new_free_block) {
            fill_bot = reinterpret_cast<char *>(new_free_block);
            f->Size += new_free_block->Size;
            this->FreeBlockList.Remove(new_free_block);
            new_free_block = f;
        }

        if (this->DebugFillEnabled) {
            if (bMemoryRandomFillPattern != 0) {
                bMemSet(fill_bot, bGetTicker(), fill_top - fill_bot);
            } else {
#if defined(MILESTONE_BUILD) && !defined(EA_BUILD_A124)
                SetFancyStompDetector(fill_bot, fill_top - fill_bot, debug_name);
#else
                bMemSet(fill_bot, 0xee, fill_top - fill_bot);
#endif
            }
        }

        this->Mutex.Unlock();
    }
}

void *MemoryPool::AllocateMemory(int size, int alignment, int alignment_offset, int start_from_top, int use_best_fit, int *new_size) {
    this->Mutex.Lock();

    size = (size + 3) & ~3;
    if (static_cast<int>(size) < static_cast<int>(sizeof(FreeBlock))) {
        size = sizeof(FreeBlock);
    }

    FreeBlock *best_free_block = nullptr;
    int best_alignment_adjust = 0;
    int best_amount_leftover = 0x7fffffff;

    if (start_from_top != 0) {
        for (FreeBlock *f = this->FreeBlockList.GetHead(); f != this->FreeBlockList.EndOfList(); f = f->GetNext()) {
            int alignment_adjust = GetAlignmentAdjustTop(reinterpret_cast<intptr_t>(f), alignment, alignment_offset);
            int amount_leftover = f->Size - (size + alignment_adjust);

            if (((amount_leftover == 0) || (amount_leftover >= static_cast<int>(sizeof(FreeBlock)))) && (amount_leftover < best_amount_leftover)) {
                best_amount_leftover = amount_leftover;
                best_alignment_adjust = alignment_adjust;
                best_free_block = f;
                if (use_best_fit == 0) {
                    break;
                }
            }
        }
    } else {
        for (FreeBlock *f = this->FreeBlockList.GetTail(); f != this->FreeBlockList.EndOfList(); f = f->GetPrev()) {
            int alignment_adjust =
                GetAlignmentAdjustBottom(reinterpret_cast<intptr_t>(reinterpret_cast<char *>(f) + f->Size) - size, alignment, alignment_offset);
            int amount_leftover = f->Size - (size + alignment_adjust);

            if (((amount_leftover == 0) || (amount_leftover >= static_cast<int>(sizeof(FreeBlock)))) && (amount_leftover < best_amount_leftover)) {
                best_amount_leftover = amount_leftover;
                best_alignment_adjust = alignment_adjust;
                best_free_block = f;
                if (use_best_fit == 0) {
                    break;
                }
            }
        }
    }

    if (best_free_block == nullptr) {
        this->Mutex.Unlock();
        return nullptr;
    }

    size += best_alignment_adjust;

    void *mem_bottom;
    if (start_from_top == 0) {
        mem_bottom = reinterpret_cast<char *>(best_free_block->GetTop()) - size;
        best_free_block->Size -= size;

        if (best_free_block->Size == 0) {
            FreeBlockList.Remove(best_free_block);
            CheckFancyStompDetector(reinterpret_cast<FreeBlock *>(mem_bottom) + 1, size - sizeof(FreeBlock));
        } else {
            CheckFancyStompDetector(mem_bottom, size);
        }
    } else {
        mem_bottom = best_free_block;
        CheckFancyStompDetector(best_free_block + 1, size - sizeof(FreeBlock));

        if (best_free_block->Size - size != 0) {
            FreeBlock *new_free_block = reinterpret_cast<FreeBlock *>(reinterpret_cast<char *>(best_free_block) + size);
            CheckFancyStompDetector(new_free_block, sizeof(FreeBlock));

            new_free_block->Size = best_free_block->Size - size;
            new_free_block->MagicNumber = 0x44443333;

            FreeBlockList.AddAfter(best_free_block, new_free_block);
        }

        FreeBlockList.Remove(best_free_block);
    }

    if (this->DebugFillEnabled) {
        if (bMemoryRandomFillPattern != 0) {
            bMemSet(mem_bottom, bGetTicker(), size);
        } else {
            bMemSet(mem_bottom, 0xaa, size);
        }
    }

    // TODO milestone
    if (bIsBFunkAvailable()) {
        // TraceAllocateMemory()
    }

    this->NumAllocations++;
    this->TotalNumAllocations++;
    this->AmountAllocated += size;
    if (this->AmountAllocated > this->MostAmountAllocated) {
        this->MostAmountAllocated = this->AmountAllocated;
    }
    this->AmountFree = this->PoolSize - this->AmountAllocated;
    this->LeastAmountFree = this->PoolSize - this->MostAmountAllocated;

    this->Mutex.Unlock();
    *new_size = size;
    return mem_bottom;
}

int MemoryPool::GetAmountFree() {
    int amount_free = 0;

    this->Mutex.Lock();
    for (FreeBlock *f = this->FreeBlockList.GetHead(); f != this->FreeBlockList.EndOfList(); f = f->GetNext()) {
        amount_free += f->Size;
    }
    this->Mutex.Unlock();
    return amount_free;
}

int MemoryPool::GetLargestFreeBlock() {
    int largest_block = 0;

    this->Mutex.Lock();
    for (FreeBlock *f = this->FreeBlockList.GetHead(); f != this->FreeBlockList.EndOfList(); f = f->GetNext()) {
        if (f->Size > largest_block) {
            largest_block = f->Size;
        }
    }
    this->Mutex.Unlock();
    return largest_block;
}

void MemoryPool::VerifyPoolIntegrity(bool verify_free_pattern) {
    this->Mutex.Lock();

    int errors = 0;
    FreeBlock *previous_f = this->FreeBlockList.GetHead();
    FreeBlock *f;
    for (f = previous_f; f != this->FreeBlockList.EndOfList(); f = f->GetNext()) {
        if (!bIsValidPointer(f, 1) || !bIsValidPointer(f->GetNext(), 1) || !bIsValidPointer(f->GetPrev(), 1) || (f->MagicNumber != 0x44443333)) {
            errors++;
        } else {
            static bool found_memory_stomp_once = false;
            if (found_memory_stomp_once) {
                break;
            }

            if (verify_free_pattern && this->DebugFillEnabled && (bMemoryRandomFillPattern == 0)) {
#if defined(MILESTONE_BUILD) && !defined(EA_BUILD_A124)
                CheckFancyStompDetector(f->GetBot(), reinterpret_cast<char *>(f->GetTop()) - reinterpret_cast<char *>(f->GetBot()));
#else
                int *bot = reinterpret_cast<int *>(f + 1);
                int *top = reinterpret_cast<int *>(f->GetTop());

                while (bot != top) {
                    if (*bot != static_cast<int>(0xeeeeeeee)) {
                        found_memory_stomp_once = true;
                        errors++;
                        break;
                    }
                    bot++;
                }
#endif
            }
        }

        if (errors != 0) {
            break;
        }
    }

    if (errors != 0) {
        bBreak();
        PrintAllocationsByAddress(0, 0x7fffffff);
        if (errors != 0) {
            goto asd;
        }
    }

    // TODO dwarf (previous_header)
    for (AllocationHeader *header = this->AllocationHeaderList.GetHead(); header != this->AllocationHeaderList.EndOfList();
         header = header->GetNext()) {
        if (!bIsValidPointer(header, 1) || !bIsValidPointer(header->GetNext(), 1) || !bIsValidPointer(header->GetPrev(), 1)) {
            errors++;
        } else if (header->MagicNumber != 0x22) {
            errors++;
        }

        if (errors != 0) {
            bBreak();
            break;
        }
    }

asd:
    this->Mutex.Unlock();
}

const char *pTraceDebugText = nullptr;  // size: 0x4, address: 0x80416450
int TraceDebugLine = 0;                 // size: 0x4, address: 0x80416454
int MemoryInitialized = 0;              // size: 0x4, address: 0x80416458
#ifdef EA_PLATFORM_PLAYSTATION2
int PS2MemorySize = 0x2000000;
void *_HeapBasePS2 = nullptr;
unsigned int _HeapSizePS2 = 0;
#endif
char MemoryPoolMem[16][sizeof(MemoryPool)]; // size: 0x600, address: 0x8045A20D
MemoryPool *MemoryPools[16];            // size: 0x40, address: 0x8045A810
MemoryPoolInfo MemoryPoolInfoTable[16]; // size: 0x100, address: 0x8045A850
bVirtualMemoryManager eARAMMM;          // size: 0x18, address: 0x8045A950
#ifndef EA_PLATFORM_PLAYSTATION2 // las dos PS2 no lo tienen (ni el global ni su guardado en bInitMemoryPool)
unsigned int MemoryPoolZeroSize = 0;    // size: 0x4, address: 0x8041645C
#endif
int bMemoryPersistentPoolNumber = -1;   // size: 0x4, address: 0x80416460

// STRIPPED
int MemoryPool::CountAllocations(const char *debug_text) {}

int CheckFlipMemoryByAddress(AllocationHeader *a, AllocationHeader *b) {
    return static_cast<int>(b->GetBottomAddress() >= a->GetBottomAddress());
}

int CheckFlipMemoryByAllocationNumber(AllocationHeader *a, AllocationHeader *b) {
#ifdef EA_BUILD_A124
    return static_cast<int>(b->GetAllocationNumber() >= a->GetAllocationNumber());
#else
    return 1;
#endif
}

void MemoryPool::PrintAllocationsByAddress(int from_allocation, int to_allocation) {
    this->AllocationHeaderList.Sort(CheckFlipMemoryByAddress);
    bReleasePrintf("\nMemoryPool: \"%s\"\n", GetName());
    bReleasePrintf("AllocationNumber Address      Size    Debug Text (& Line)\n");
    bReleasePrintf("=========================================================\n");

    AllocationHeader *header;
    AllocationHeader *prev_header = nullptr;
    int free_mem;
    for (header = this->AllocationHeaderList.GetHead(); header != this->AllocationHeaderList.EndOfList(); header = header->GetNext()) {
        if (header->GetAllocationNumber() < from_allocation) {
            prev_header = nullptr;
        } else if (header->GetAllocationNumber() < to_allocation) {
            if (prev_header != nullptr) {
                unsigned char *prev_header_bot = static_cast<unsigned char *>(prev_header->GetBottomAddress());
                unsigned char *header_bot = static_cast<unsigned char *>(header->GetBottomAddress());
                int block_size = header_bot - prev_header_bot;
                int gap_size = block_size - prev_header->Size;
                if (gap_size != 0) {
                    bReleasePrintf("     *************** FREE    %6d **************\n", gap_size);
                }
            }

            bReleasePrintf("    %5d        0x%08x %7d   %s", header->GetAllocationNumber(), header->GetBottomAddress(), header->Size,
                           header->GetDebugText());
            if (header->GetDebugLine() != 0) {
                bReleasePrintf(", %d", header->GetDebugLine());
            }
            bReleasePrintf("\n");
            prev_header = header;
        } else {
            prev_header = nullptr;
        }
    }
}

void MemoryPool::PrintAllocations(int from_allocation, int to_allocation) {
    this->AllocationHeaderList.Sort(CheckFlipMemoryByAllocationNumber);
    bReleasePrintf("\nMemoryPool: \"%s\"\n", this->GetName());
    bReleasePrintf("AllocationNumber Address      Size    Debug Text (& Line)\n");
    bReleasePrintf("=========================================================\n");

    for (AllocationHeader *header = this->AllocationHeaderList.GetHead(); header != this->AllocationHeaderList.EndOfList();
         header = header->GetNext()) {
        if ((header->GetAllocationNumber() >= from_allocation) && (header->GetAllocationNumber() < to_allocation)) {
            bReleasePrintf("    %5d        0x%08x %7d   %s", header->GetAllocationNumber(), header->GetBottomAddress(), header->Size,
                           header->GetDebugText());
            if (header->GetDebugLine() != 0) {
                bReleasePrintf(", %d", header->GetDebugLine());
            }
            bReleasePrintf("\n");
        }
    }
}

// STRIPPED
AllocationHeader *MemoryPool::FindAllocation(int allocation_num) {
    for (AllocationHeader *header = this->AllocationHeaderList.GetHead(); header != this->AllocationHeaderList.EndOfList();
         header = header->GetNext()) {
        if (header->GetAllocationNumber() == allocation_num) {
            return header;
        }
    }
    return nullptr;
}

int MemoryPool::GetAllocations(void **allocations, int max_allocations) {
    this->AllocationHeaderList.Sort(CheckFlipMemoryByAddress);
    int num_allocations = 0;
    for (AllocationHeader *header = this->AllocationHeaderList.GetHead(); header != this->AllocationHeaderList.EndOfList();
         header = header->GetNext()) {
        if (num_allocations < max_allocations) {
            allocations[num_allocations] = header->GetAllocAddress();
            num_allocations++;
        }
    }
    return num_allocations;
}

#ifdef EA_BUILD_A124
void *MemoryPool::GetMemoryDumpStatistics(int &num_allocs, int &high_allocs, int &usage, int &high_usage, int &free, const char *&name) {
    num_allocs = NumAllocations;
    high_allocs = TotalNumAllocations;
    usage = AmountAllocated;
    high_usage = MostAmountAllocated;
    free = AmountFree;
    name = GetName();

    return AllocationHeaderList.GetHead();
}
#endif

#if defined(MILESTONE_BUILD) && !defined(EA_BUILD_A124)
// X360 (NFS.exe 0x8216D178): la memoria libre se llena con el nombre de quien la libero, de tres en tres
// caracteres mas su suma; "NULL" si no hay nombre.
void MemoryPool::SetFancyStompDetector(void *mem, int mem_size, const char *name) {
    if (name == nullptr) {
        name = "NULL";
    }
    int name_pos = 0;
    for (int n = 0; n < mem_size; n += 4) {
        unsigned char *pmem = &static_cast<unsigned char *>(mem)[n];
        unsigned char c0 = name[name_pos++];
        if (c0 == 0) {
            name_pos = 0;
        }
        unsigned char c1 = name[name_pos++];
        if (c1 == 0) {
            name_pos = 0;
        }
        unsigned char c2 = name[name_pos++];
        if (c2 == 0) {
            name_pos = 0;
        }
        unsigned char checksum = c0 + c1 + c2;
        pmem[0] = c0;
        pmem[1] = c1;
        pmem[2] = c2;
        pmem[3] = checksum;
    }
}
#else
// STRIPPED
void MemoryPool::SetFancyStompDetector(void *mem, int mem_size, const char *name) {}
#endif

bool MemoryPool::CheckFancyStompDetector(const void *mem, int mem_size) {
#if defined(MILESTONE_BUILD) && !defined(EA_BUILD_A124)
    const unsigned char *mem8 = static_cast<const unsigned char *>(mem);

    if (this->DebugFillEnabled && bMemoryRandomFillPattern == 0) {
        for (int n = 0; n < mem_size; n += 4) {
            const unsigned char *pmem = &mem8[n];
            unsigned char c0 = pmem[0];
            unsigned char c1 = pmem[1];
            unsigned char c2 = pmem[2];
            unsigned char checksum = c0 + c1 + c2;

            if (checksum != pmem[3]) {
                bReleasePrintf("\nERROR:  FancyStompDetector detected stomp at 0x%08x in memory pool %s\n", pmem, GetName());
                bReleasePrintf("        Bytes 0,1,2 = previous owner (ASCII).  Byte 3 = checksum\n");
                bReleasePrintf("\n");
                int start_pos = (n & ~0xF) - 0x20;
                int end_pos = start_pos + 0x60;
                for (int pos = start_pos; pos < end_pos; pos += 0x10) {
                    bReleasePrintf("        0x%08X : ", &mem8[pos]);
                    for (int i = 0; i < 0x10; i++) {
                        bReleasePrintf(" %02X", mem8[pos + i]);
                    }
                    bReleasePrintf("  ");
                    for (int i = 0; i < 0x10; i++) {
                        if ((i & 3) != 3) {
                            unsigned char c = mem8[pos + i];
                            if (c < 0x20 || c >= 0x80) {
                                c = '.';
                            }
                            bReleasePrintf("%c", c);
                        }
                    }
                    bReleasePrintf("\n");
                }
                PrintAllocationsByAddress(0, 0x7fffffff);
                static int seen_yellow_screen;
                seen_yellow_screen++;
                if (bMemoryBreakOnFancyStompDetection) {
                    bBreak();
                }
                return true;
            }
        }
    }
#endif
    return false;
}

void MemoryPool::TraceNewPool() {
    bMemoryTraceNewPoolPacket packet;
    packet.PoolID = reinterpret_cast<uintptr_t>(this);
    bMemSet(packet.Name, 0, sizeof(packet.Name));
    if (this->pDebugName != nullptr) {
        bStrNCpy(packet.Name, this->pDebugName, sizeof(packet.Name) - 1);
    }
    // TODO probably macro instead of ifdefs here
    // TODO apply macro for "25"
#ifdef EA_PLATFORM_GAMECUBE
    bFunkGameCube("CODEINE", 25, &packet, sizeof(packet));
#else
    bFunkCallASync("CODEINE", 25, &packet, sizeof(packet));
#endif
}

void MemoryPool::TraceDeletePool() {
    bMemoryTraceDeletePoolPacket packet;
    packet.PoolID = reinterpret_cast<uintptr_t>(this);
#ifdef EA_PLATFORM_GAMECUBE
    bFunkGameCube("CODEINE", 26, &packet, sizeof(packet));
#else
    bFunkCallASync("CODEINE", 26, &packet, sizeof(packet));
#endif
}

void MemoryPool::TraceFreeMemory(void *p, int size) {
    bMemoryTraceFreePacket packet = {0};

    packet.PoolID = reinterpret_cast<uintptr_t>(this);
    packet.MemoryAddress = reinterpret_cast<uintptr_t>(p);
    packet.Size = size;
#ifdef EA_PLATFORM_GAMECUBE
    bFunkGameCube("CODEINE", 27, &packet, sizeof(packet));
#else
    bFunkCallASync("CODEINE", 27, &packet, sizeof(packet));
#endif
}

// STRIPPED
void MemoryPool::TraceRemoveMemory(void *p, int size) {}

// STRIPPED
void TrapMissingMemoryTraces(int size) {}

#ifdef EA_PLATFORM_PLAYSTATION2
void MemoryPool::TraceAllocateMemory(void *p, int size) {
    bMemoryTraceAllocatePacket packet = {reinterpret_cast<intptr_t>(this), reinterpret_cast<intptr_t>(p), size, TraceDebugLine,
                                         bMemoryAllocationNumber};

    bMemSet(packet.DebugText, 0, sizeof(packet.DebugText));
    if (pTraceDebugText) {
        bStrNCpy(packet.DebugText, pTraceDebugText, sizeof(packet.DebugText) - 1);
    }
    int unused_text_size = sizeof(packet.DebugText) - 1 - bStrLen(packet.DebugText);
    bFunkCallASync("CODEINE", 28, &packet, sizeof(packet) - unused_text_size);
    if (pTraceDebugText) {
        if (!pTraceDebugText[0]) {
            TrapMissingMemoryTraces(size);
        }
    }
    pTraceDebugText = nullptr;
    TraceDebugLine = 0;
}

void MemoryPool::UpdateTraceInformation() {
    if (DebugTracingEnabled && bIsCodeineConnected()) {
        TraceNewPool();
        for (FreeBlock *free_block = static_cast<FreeBlock *>(FreeBlockList.GetHead()); free_block != FreeBlockList.EndOfList();
             free_block = free_block->GetNext()) {
            TraceFreeMemory(free_block, free_block->Size);
        }
        int saved_allocation_number = bMemoryAllocationNumber;
        for (AllocationHeader *header = static_cast<AllocationHeader *>(AllocationHeaderList.GetHead());
             header != AllocationHeaderList.EndOfList(); header = header->GetNext()) {
#ifdef MILESTONE_BUILD
            pTraceDebugText = header->GetDebugText();
            TraceDebugLine = 0;
            bMemoryAllocationNumber = header->GetAllocationNumber();
#endif
            TraceAllocateMemory(header->GetBottomAddress(), header->Size);
        }
        bMemoryAllocationNumber = saved_allocation_number;
    }
}
#else
// STRIPPED
void MemoryPool::TraceAllocateMemory(void *p, int size) {}

// STRIPPED
void MemoryPool::UpdateTraceInformation() {}
#endif

int bGetFreeMemoryPoolNum() {
    for (int pool_num = 0; pool_num < 16; pool_num++) {
        MemoryPoolInfo *info = &MemoryPoolInfoTable[pool_num];
        if (!info->NumberReserved && (MemoryPools[pool_num] == nullptr) && (info->OverrideInfo == nullptr)) {
            return pool_num;
        }
    }
    return -1;
}

void bReserveMemoryPool(int pool_num) {
    MemoryPoolInfo *info = &MemoryPoolInfoTable[pool_num];
    info->NumberReserved = true;
}

void bSetMemoryPoolOverrideInfo(int pool_num, MemoryPoolOverrideInfo *override_info) {
    MemoryPoolInfo *info = &MemoryPoolInfoTable[pool_num];
    info->OverrideInfo = override_info;
    info->OverflowPoolNumber = -1;
}

void bInitMemoryPool(int pool_num, void *mem, int mem_size, const char *debug_name) {
    MemoryPoolInfo *info = &MemoryPoolInfoTable[pool_num];
    info->TopMeansLargerAddress = false;
    info->OverflowPoolNumber = -1;
    MemoryPools[pool_num] = reinterpret_cast<MemoryPool *>(MemoryPoolMem[pool_num]);
    reinterpret_cast<MemoryPool *>(MemoryPoolMem[pool_num])->Init(mem, mem_size, debug_name);
#if !defined(EA_PLATFORM_PLAYSTATION2) && !defined(EA_PLATFORM_XENON) // NFS.exe 0x8216D5E8 no lo guarda
    if (pool_num == 0) {
        MemoryPoolZeroSize = mem_size;
    }
#endif
}

void bCloseMemoryPool(int pool_num) {
    MemoryPools[pool_num]->Close();
    MemoryPools[pool_num] = nullptr;
}

bool bSetMemoryPoolDebugFill(int pool_num, bool on_off) {
    return MemoryPools[pool_num]->SetDebugFill(on_off);
}

// STRIPPED
void bAddToMemoryPool(int pool_num, void *mem, int mem_size) {}

// STRIPPED
void bRemoveFromMemoryPool(int pool_num, void *mem, int mem_size) {}

bool bSetMemoryPoolDebugTracing(int pool_num, bool on_off) {
    bool previous;

    if (!on_off) {
        void *dummy = bMalloc(0x10, "Tracing disabled for this pool", 0, (pool_num & 0xf) | 0x40);

        previous = MemoryPools[pool_num]->SetDebugTracing(false);
        bFree(dummy);
    } else {
        previous = MemoryPools[pool_num]->SetDebugTracing(on_off);
    }
    return previous;
}

void bSetMemoryPoolTopDirection(int pool_num, bool top_means_larger_address) {
    MemoryPoolInfo *info = &MemoryPoolInfoTable[pool_num];
    info->TopMeansLargerAddress = top_means_larger_address;
}

void bMemorySetOverflowPoolNumber(int pool_num, int overflow_pool_number) {
    MemoryPoolInfo *info = &MemoryPoolInfoTable[pool_num];
    info->OverflowPoolNumber = overflow_pool_number;
}

int PlatformMemoryINIT() {
    return -1;
}

void bVirtualMemoryManager::Init() {
#ifdef EA_PLATFORM_GAMECUBE
    this->bIsValid = FALSE;
    this->mVirtualBaseAddr = 0x7e000000;
    this->mMRamBaseAddr = reinterpret_cast<uintptr_t>(OSGetArenaLo());
    this->mMRamSize = 0x80000;
    const uintptr_t end_of_audio_aram = ARGetBaseAddress() + 0x8010ffU & ~0xfff;
    this->mARamSize = 0x600000;
    this->mARamBaseAddr = end_of_audio_aram;
    VMInit(this->mMRamSize, end_of_audio_aram, this->mARamSize);
#else
// TODO
#endif
}

// STRIPPED
void bVirtualMemoryManager::Quit() {}

void bVirtualMemoryManager::Alloc() {
    this->bIsValid = VMAlloc(this->mVirtualBaseAddr, this->mARamSize);
}

const char *GetVirtualMemoryPoolName() {
    return "NGC_VirtualMemory";
}

int GetVirtualMemoryPoolNumber() {
    return 15;
}

#ifdef EA_PLATFORM_GAMECUBE
unsigned int GetVirtualMemoryAllocParams() {
    return (GetVirtualMemoryPoolNumber() & 0xf) | 0x400;
}
#endif

#ifdef EA_PLATFORM_PLAYSTATION2
// Los simbolos del enlazador del crt0 de Sony: el final del programa y la base y el tamano de la pila (-1 si no se
// fijaron).
extern char _end[];
extern char _stack[];
extern char _stack_size[];

int GetPS2HeapSize() {
    int ProgEnd = reinterpret_cast<int>(_end);
    int StackSize = reinterpret_cast<int>(_stack_size);
    if (StackSize == -1) {
        StackSize = 0x4000;
    }
    int StackStart = reinterpret_cast<int>(_stack);
    if (StackStart == -1) {
        StackStart = PS2MemorySize - StackSize;
    }
    return StackStart - ProgEnd;
}
#endif

void bMemoryInit() {
#ifdef EA_PLATFORM_GAMECUBE
    void *arenaLo;
    void *arenaHi;
    if (!MemoryInitialized) {
        int nPMem = PlatformMemoryINIT();
        arenaLo = OSGetArenaLo();
        arenaHi = OSGetArenaHi();

        OSSetArenaLo(OSInitAlloc(arenaLo, arenaHi, 1));
        eARAMMM.Init();
        eARAMMM.Alloc();

        arenaLo = OSGetArenaLo();
        arenaHi = OSGetArenaHi();
        arenaLo = reinterpret_cast<void *>((reinterpret_cast<uintptr_t>(arenaLo) + 0x1f) & 0xffffffe0);
        arenaHi = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(arenaHi) & 0xffffffe0);
        int nBMem = static_cast<char *>(arenaHi) - static_cast<char *>(arenaLo);

        if ((nPMem < 1) || (nBMem <= nPMem)) {
            nPMem = nBMem;
        }

        OSSetCurrentHeap(OSCreateHeap(arenaLo, arenaHi));
        OSSetArenaLo(arenaHi);

        if (bMemoryTracing != 0) {
            bFunkGameCube("CODEINE", 24, nullptr, 0);
        }

        int bware_memory_size = nPMem - 0x40000;
        void *main_pool = OSAlloc(bware_memory_size);
        bInitMemoryPool(0, main_pool, bware_memory_size, "Main Pool");
        // TODO
        void *heap_bot;
        void *heap_top;
        int pool_number;
        int pool_num_VM = GetVirtualMemoryPoolNumber();
        bInitMemoryPool(pool_num_VM, reinterpret_cast<void *>(eARAMMM.mVirtualBaseAddr), eARAMMM.mARamSize, GetVirtualMemoryPoolName());
        MemoryInitialized = TRUE;
    }
#elif defined(EA_PLATFORM_PLAYSTATION2)
    if (!MemoryInitialized) {
        int heap_size = GetPS2HeapSize() - 0x8000;
        void *heap_bot = malloc(heap_size);
        if (bMemoryTracing != 0) {
            bFunkCallASync("CODEINE", 24, nullptr, 0);
        }
        bInitMemoryPool(0, heap_bot, heap_size, "Main Pool");
        _HeapBasePS2 = heap_bot;
        _HeapSizePS2 = heap_size;
        MemoryInitialized = TRUE;
    }
#elif defined(EA_PLATFORM_XENON)
    // NFS.exe 0x82170F70: la piscina principal son 0x15900000 B fisicos en paginas grandes; el estado de la memoria
    // se lee antes y despues (GlobalMemoryStatus, 0x82669A28) y no se usa.
    if (!MemoryInitialized) {
        MEMORYSTATUS memory_status_before;
        GlobalMemoryStatus(&memory_status_before);
        int heap_size = 0x15900000;
        void *heap_bot = XPhysicalAlloc(heap_size, MAXULONG_PTR, 0, PAGE_READWRITE | MEM_LARGE_PAGES);
        MEMORYSTATUS memory_status_after;
        GlobalMemoryStatus(&memory_status_after);
        bInitMemoryPool(0, heap_bot, heap_size, "Main Pool");
        MemoryInitialized = TRUE;
    }
#else
    // TODO
#endif
}

#ifdef EA_PLATFORM_PLAYSTATION2
void bMemoryUpdateTraceInformation() {
    if (bMemoryTracing) {
        bFunkCallASync("CODEINE", 0x18, nullptr, 0);
        for (int pool_num = 0; pool_num < 16; pool_num++) {
            if (MemoryPools[pool_num]) {
                MemoryPools[pool_num]->UpdateTraceInformation();
            }
        }
    }
}
#else
// STRIPPED
void bMemoryUpdateTraceInformation() {}
#endif

// La alpha 124 es milestone y aun asi define las dos, primero la de dos argumentos.
#if !defined(MILESTONE_BUILD) || defined(EA_BUILD_A124)
void *bMalloc(int size, int allocation_params) {
    return bWareMalloc(size, nullptr, 0, allocation_params);
}
#endif

#ifdef EA_BUILD_A124
void *bMalloc(int size, const char *debug_text, int debug_line, int allocation_params) {
    return bWareMalloc(size, debug_text, debug_line, allocation_params);
}
#endif

// TODO variable names
#if defined(MILESTONE_BUILD) && !defined(EA_BUILD_A124)
// X360 no tiene bWareMalloc: el cuerpo es el de bMalloc (NFS.exe 0x8216D8C8, 968 B) y sus 1.365 llamantes lo llaman.
void *bMalloc(int size, const char *debug_text, int debug_line, int allocation_params) {
#else
void *bWareMalloc(int size, const char *debug_text, int debug_line, int allocation_params) {
#endif
    int pool_num = bMemoryGetPoolNum(allocation_params);
    MemoryPoolInfo *info = &MemoryPoolInfoTable[pool_num];

    if (info->OverflowPoolNumber != -1) {
        int overflow_allocation_params = (allocation_params & ~0xf) | (info->OverflowPoolNumber & 0xf);
        if (bLargestMalloc(overflow_allocation_params) > size) {
            info = &MemoryPoolInfoTable[info->OverflowPoolNumber];
            allocation_params = overflow_allocation_params;
        }
    }

    if (info->TopMeansLargerAddress) {
        allocation_params ^= 0x40;
    }

    MemoryPoolOverrideInfo *override_info = info->OverrideInfo;
    if (override_info != nullptr) {
        return override_info->Malloc(override_info->Pool, size, debug_text, debug_line, allocation_params);
    }

    pTraceDebugText = debug_text;
    TraceDebugLine = debug_line;
    if (MemoryInitialized == 0) {
        bMemoryInit();
    }

    if (bMemoryAutomaticVerifyPoolIntegrity != 0) {
        if (bMemoryAllocationNumber % bMemoryAutomaticVerifyPoolIntegrity == 0) {
            bVerifyPoolIntegrity(bMemoryGetPoolNum(allocation_params));
        }
    }

    // TODO rename
#ifdef MILESTONE_BUILD
    int alignment = bMemoryGetAlignment(allocation_params) ? bMemoryGetAlignment(allocation_params) : 16;
#else
    int alignment = bMemoryGetAlignment(allocation_params);
    if (alignment == 0) {
        alignment = 16;
    }
#endif

    // TODO rename
#ifdef MILESTONE_BUILD
    int user_alignment_offset = bMemoryGetAlignmentOffset(allocation_params);
    int shared_string_index = -1;
    if (bMemoryUseSharedStrings) {
        shared_string_index = bGetSharedStringIndex(bAllocateSharedString(debug_text));
    }
    int debug_header_size = DebugAllocationHeader::GetRequiredSize(debug_text, shared_string_index);
    int allocation_header_offset = debug_header_size + sizeof(AllocationHeader) + user_alignment_offset;
    int new_size;
    pool_num = bMemoryGetPoolNum(allocation_params);
    MemoryPool *pool = MemoryPools[pool_num];
    char *memory = reinterpret_cast<char *>(pool->AllocateMemory(size + sizeof(AllocationHeader) + debug_header_size, alignment,
                                                                 allocation_header_offset, allocation_params & 0x40, allocation_params & 0x80, &new_size));
#else
    int allocation_header_offset = bMemoryGetAlignmentOffset(allocation_params) + static_cast<int>(sizeof(AllocationHeader));
    int new_size;
    pool_num = bMemoryGetPoolNum(allocation_params);
    MemoryPool *pool = MemoryPools[pool_num];
    void *memory =
        pool->AllocateMemory(size + static_cast<int>(sizeof(AllocationHeader)), alignment, allocation_header_offset, allocation_params & 0x40, allocation_params & 0x80, &new_size);
#endif

    if (memory != nullptr) {
        int padding = 0;
        int front_padding; // TODO
        if (allocation_params & 0x40) {
            padding = GetAlignmentAdjustTop(reinterpret_cast<intptr_t>(memory), alignment, allocation_header_offset);
        }

#ifdef MILESTONE_BUILD
        front_padding = debug_header_size + padding;
        AllocationHeader *header = reinterpret_cast<AllocationHeader *>(memory + front_padding);
        pool->AddAllocationHeader(header);

        header->PoolNum = pool_num;
        header->FrontPadding = front_padding;
        header->MagicNumber = 0x22;
        header->Size = new_size;
        header->RequestedSize = size;

        reinterpret_cast<DebugAllocationHeader *>(memory)->DebugLine = debug_line;
        reinterpret_cast<DebugAllocationHeader *>(memory)->AllocationNumber = bMemoryAllocationNumber;
        reinterpret_cast<DebugAllocationHeader *>(memory)->SharedStringIndex = shared_string_index;
        reinterpret_cast<DebugAllocationHeader *>(memory)->DebugText[0] = 0;
        if (debug_text && shared_string_index == -1) {
            bStrCpy(reinterpret_cast<DebugAllocationHeader *>(memory)->DebugText, debug_text);
        }
        bMemSet(memory + debug_header_size, 0xDD, header->FrontPadding - debug_header_size);
        if (bMemoryAllocCallback) {
#ifdef EA_BUILD_A124
            bMemoryAllocCallback(header->PoolNum, &header[1], size, reinterpret_cast<DebugAllocationHeader *>(memory)->DebugText);
#else // X360: NFS.exe 0x8216D8C8 llama a GetDebugText
            bMemoryAllocCallback(header->PoolNum, &header[1], size, header->GetDebugText());
#endif
        }
#else
        AllocationHeader *header = reinterpret_cast<AllocationHeader *>(reinterpret_cast<char *>(memory) + padding);
        pool->AddAllocationHeader(header);

        header->PoolNum = pool_num;
        header->MagicNumber = 0x22;
        header->FrontPadding = padding;

        header->Size = new_size;
        header->RequestedSize = size;
#endif
        if (bMemoryAllocationNumber == bMemoryBreakOnAllocationNumber) {
            bBreak();
        }

        bMemoryAllocationNumber++;
        return &header[1];
    }

    bReleasePrintf("ERROR:  Out of memory in pool %s allocating %s (size = %d).  Largest possible = %d  Total = %d\n", pool->GetName(), debug_text,
                   size, bLargestMalloc(allocation_params), bCountFreeMemory(pool_num));
#ifdef MILESTONE_BUILD
    bBreak();
#endif
    bMemoryPrintAllocationsByAddress(pool_num, 0, 0x7fffffff);
    bBreak();
    return nullptr;
}

void bFree(void *ptr) {
    if (ptr == nullptr) {
        return;
    }
    for (int n = 0; n < 16; n++) {
        MemoryPoolOverrideInfo *override_info = MemoryPoolInfoTable[n].OverrideInfo;
        intptr_t address = reinterpret_cast<intptr_t>(ptr);
        if ((override_info != nullptr) && (address >= override_info->Address) && (address < override_info->Address + override_info->Size)) {
            int pool_num;

            for (pool_num = 1; pool_num < 16; pool_num++) {
                MemoryPool *pool = MemoryPools[pool_num];

                if ((pool != nullptr) && pool->IsInPool(address)) {
                    break;
                }
            }
            if (pool_num == 16) {
                override_info->Free(override_info->Pool, ptr);
                return;
            }
        }
    }
    AllocationHeader *header = &static_cast<AllocationHeader *>(ptr)[-1];
    int pool_num = header->PoolNum;
    MemoryPool *pool = MemoryPools[pool_num];
#if defined(MILESTONE_BUILD) && !defined(EA_BUILD_A124)
    char debug_name[32] = "";
#elif !defined(EA_BUILD_A124)
    char debug_name[32] = {};
#endif
#ifdef MILESTONE_BUILD
#ifndef EA_BUILD_A124
    DebugAllocationHeader *debug_header = header->GetDebugHeader();
    bSafeStrCpy(debug_name, header->GetDebugText(), sizeof(debug_name));
#endif
    if (bMemoryFreeCallback) {
        bMemoryFreeCallback(header->PoolNum, ptr, header->RequestedSize, header->GetDebugText());
    }
#ifdef EA_BUILD_A124
    if (header->GetDebugHeader()->SharedStringIndex != -1) {
        bFreeSharedString(bGetSharedString(header->GetDebugHeader()->SharedStringIndex));
    }
#else
    bFreeSharedString(bGetSharedString(debug_header->SharedStringIndex));
#endif
#endif
    if (bMemoryAutomaticVerifyPoolIntegrity && (bMemoryAllocationNumber % bMemoryAutomaticVerifyPoolIntegrity == 0)) {
        bVerifyPoolIntegrity(pool_num);
    }
    if (header->MagicNumber != 0x22) {
        bBreak();
    } else {
        void *allocated_pointer = header->GetBottomAddress();
        header->MagicNumber = 0;
        pool->RemoveAllocationHeader(header);
        pool->FreeMemory(allocated_pointer, header->Size
#ifndef EA_BUILD_A124
                         ,
                         debug_name
#endif
        );
    }
}

size_t bGetMallocSize(const void *ptr) {
    if (ptr != nullptr) {
        const AllocationHeader *header = &static_cast<const AllocationHeader *>(ptr)[-1];
        return header->RequestedSize;
    }
    return 0;
}

int bGetMallocPool(void *ptr) {
    if (ptr != nullptr) {
        AllocationHeader *header = &static_cast<AllocationHeader *>(ptr)[-1];
        return header->PoolNum;
    }
    return 0;
}

// STRIPPED
int bGetMallocNumber(void *ptr) {
    if (ptr != nullptr) {
        AllocationHeader *header = &static_cast<AllocationHeader *>(ptr)[-1];
        return header->GetAllocationNumber();
    }
    return 0;
}

const char *bGetMallocName(void *ptr) {
    if (ptr != nullptr) {
        AllocationHeader *header = &static_cast<AllocationHeader *>(ptr)[-1];
        return header->GetDebugText();
    }
    return nullptr;
}

int bCountFreeMemory(int pool) {
    if (MemoryPools[pool] == nullptr) {
        MemoryPoolOverrideInfo *override_info = MemoryPoolInfoTable[pool].OverrideInfo;
        if (override_info != nullptr) {
            return override_info->GetAmountFree(override_info->Pool);
        } else {
            return 0;
        }
    }
    return MemoryPools[pool]->GetAmountFree();
}

// STRIPPED
int bGetPoolSize(int pool) {
    if (MemoryPools[pool] != nullptr) {
        return MemoryPools[pool]->GetPoolSize();
    }

    return 0;
}

int bLargestMalloc(int allocation_params) {
    if (MemoryPools[allocation_params & 0xfU] == nullptr) {
        MemoryPoolOverrideInfo *override_info = MemoryPoolInfoTable[allocation_params & 0xfU].OverrideInfo;
        if (override_info != nullptr) {
            return override_info->GetLargestFreeBlock(override_info->Pool);
        } else {
            return 0;
        }
    }
    int pool = MemoryPools[bMemoryGetPoolNum(allocation_params)]->GetLargestFreeBlock() - 0x5c;
    int alignment = bMemoryGetAlignment(allocation_params);
    if (alignment == 0) {
        alignment = 16;
    }
    if (alignment < 128) {
        alignment = 128;
    }
    int largest_malloc = pool - alignment;
    if (largest_malloc < 0) {
        largest_malloc = 0;
    }
    return largest_malloc;
}

void bVerifyPoolIntegrity(int pool) {
    if (MemoryPools[pool] != nullptr) {
        bool verify_free_pattern = true;
        MemoryPools[pool]->VerifyPoolIntegrity(verify_free_pattern);
    }
}

// STRIPPED
const char *bGetMemoryPoolName(int pool_num) {
    if (MemoryPools[pool_num] != nullptr) {
        return MemoryPools[pool_num]->GetName();
    }

#ifndef EA_BUILD_A124
    MemoryPoolOverrideInfo *override_info = MemoryPoolInfoTable[pool_num].OverrideInfo;
    if (override_info != nullptr) {
        return override_info->Name;
    }
#endif

    return nullptr;
}

int bGetMemoryPoolNum(const char *memory_pool_name) {
    for (int pool_num = 0; pool_num < 16; pool_num++) {
        if (MemoryPools[pool_num] != nullptr) {
            if (bStrICmp(MemoryPools[pool_num]->GetName(), memory_pool_name) == 0) {
                return pool_num;
            }
        }
#ifndef EA_BUILD_A124
        MemoryPoolOverrideInfo *override_info = MemoryPoolInfoTable[pool_num].OverrideInfo;
        if ((override_info != nullptr) && bStrICmp(override_info->Name, memory_pool_name) == 0) {
            return pool_num;
        }
#endif
    }
    return -1;
}

// STRIPPED
int bMemoryCountAllocations(const char *debug_text, int pool_num) {}

int bMemoryGetAllocationNumber() {
    return bMemoryAllocationNumber;
}

void bMemoryPrintAllocations(int pool_num, int from_allocation, int to_allocation) {
    MemoryPools[pool_num]->PrintAllocations(from_allocation, to_allocation);
}

void bMemoryPrintAllocationsByAddress(int pool_num, int from_allocation, int to_allocation) {
    if (MemoryPools[pool_num] != nullptr) {
        MemoryPools[pool_num]->PrintAllocationsByAddress(from_allocation, to_allocation);
    }
}

// STRIPPED
void *bMemoryFindAllocation(int pool_num, int allocation_num) {
    if (MemoryPools[pool_num] != nullptr) {
        AllocationHeader *header = MemoryPools[pool_num]->FindAllocation(allocation_num);
        if (header != nullptr) {
            return header->GetAllocAddress();
        }
    }
    return nullptr;
}

int bMemoryGetAllocations(int pool_num, void **allocations, int max_allocations) {
    if (MemoryPools[pool_num] != nullptr) {
        return MemoryPools[pool_num]->GetAllocations(allocations, max_allocations);
    }
    return 0;
}

#ifdef EA_BUILD_A124
void *bMemoryGetStatistics(int pool, int &num_allocs, int &high_allocs, int &usage, int &high_usage, int &free, int &largest_free,
                           const char *&name) {
    MemoryPool *memory_pool = MemoryPools[pool];
    if (memory_pool) {
        void *first_allocation = memory_pool->GetMemoryDumpStatistics(num_allocs, high_allocs, usage, high_usage, free, name);
        largest_free = bLargestMalloc(pool);
        return first_allocation;
    }
    return nullptr;
}
#endif

#ifdef EA_BUILD_A124
static char bMemoryDebugStringNoName[8] = "NO_NAME";
#endif

void *bMemoryAllocator::Alloc(size_t size, const EA::TagValuePair &flags) {

#ifdef EA_BUILD_A124
    char *name = bMemoryDebugStringNoName;
#else
    char *name;
#endif
    int allocation_params = BMEMORY_TOP_BIT;
    const EA::TagValuePair *p = &flags;
    void *ptr;

    while (p != nullptr) {
        switch (p->mTag) {
            case 1:
                if (p->mValue.mPointer != nullptr) {
                    name = static_cast<char *>(const_cast<void *>(p->mValue.mPointer));
                }
                break;
            case 2:
                allocation_params |= BMEMORY_ALIGNMENT(p->mValue.mInt);
                break;
            case 3:
                allocation_params |= BMEMORY_ALIGNMENT_OFFSET(p->mValue.mInt);
                break;
            case 4:
                allocation_params &= ~BMEMORY_TOP_BIT;
                break;
        }
        p = p->mNext;
    }
    return bMalloc(size, name, 0, allocation_params);
}

// STRIPPED
void *bMemoryAllocator::Alloc(size_t size) {}

void bMemoryAllocator::Free(void *pBlock, size_t size) {
    bFree(pBlock);
}

int bMemoryAllocator::AddRef() {
    return ++this->mRefcount;
}

int bMemoryAllocator::Release() {
    int ref = --this->mRefcount;
    if (ref > 0) {
        return ref;
    }
    delete this;
    return 0;
}

void bMemoryCreatePersistentPool(int size) {
    bMemoryPersistentPoolNumber = bGetFreeMemoryPoolNum();
    void *mem = bMalloc(size, "Persistent Memory Pool", 0, 0);
    bInitMemoryPool(bMemoryPersistentPoolNumber, mem, size, "Persistent Pool");
    TheMemoryPersistentAllocator.SetMemoryPool(bMemoryPersistentPoolNumber);
}
