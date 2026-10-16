#ifndef BWARE_PSX2_FASTPACKETBUFFER_H
#define BWARE_PSX2_FASTPACKETBUFFER_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include <sif.h>

// indep/bware/src/psx2/FastPacketBuffer.hpp de la A124: el canal de paquetes entre el EE y el IOP que usa el monitor
// de red de SuperBender. Los layouts, los nombres y el orden de los metodos salen de los stabs; el de
// FastPacketBuffer (26.828 B) cuadra solo.

struct FastPacketHeader {
    unsigned int TotalSize;    // offset 0x0, size 0x4
    unsigned int DataSize;     // offset 0x4, size 0x4
    unsigned int PacketNumber; // offset 0x8, size 0x4
    unsigned int Checksum;     // offset 0xC, size 0x4
};

struct FastPacket : public FastPacketHeader {
    unsigned char Data[2048]; // offset 0x10, size 0x800
};

struct MemoryExchangeHeader {
    unsigned char DoReadDMA;  // offset 0x0, size 0x1
    unsigned char DoWriteDMA; // offset 0x1, size 0x1
    unsigned char Done;       // offset 0x2, size 0x1
    unsigned char Padding1;   // offset 0x3, size 0x1
    unsigned int Address;     // offset 0x4, size 0x4
    unsigned int Size;        // offset 0x8, size 0x4
    unsigned int Padding;     // offset 0xC, size 0x4
};

struct MemoryExchangePacket : public MemoryExchangeHeader {
    unsigned char Data[16]; // offset 0x10, size 0x10
};

// Las dos funciones de la DMA del SIF van por punteros: el lado del IOP pone las suyas.
struct FastPacketFunctionInfo {
    unsigned int (*sceSifSetDma)(sceSifDmaData *sdd, int len); // offset 0x0, size 0x4
    int (*sceSifDmaStat)(unsigned int id);                     // offset 0x4, size 0x4
};

class FastPacketBuffer {
  public:
    void Reset();
    void Start(FastPacketBuffer *other_side);
    void Stop();
    bool CanReceivePacket();
    int GetNumBytesToReceive();
    void *GetReceivePacket(int *psize);
    void AdvanceReceivePacket();
    bool CanSendPacket(int size);
    void *GetSendPacket(int size);
    void AdvanceSendPacket();
    unsigned int ReadMemory(unsigned int address);
    void WriteMemory(unsigned int address, unsigned int value);
    void ServiceMemoryExchanger();
    FastPacketFunctionInfo *GetFunctionInfo() {
        return &FunctionInfo;
    }

  private:
    void DoDma(void *source_address, void *dest_address, int size);
    void StartDma(sceSifDmaData *dma_data, int num_dma);

    unsigned int OtherNumBytesReceived;    // offset 0x0, size 0x4
    unsigned char Padding1[12];            // offset 0x4, size 0xC
    unsigned int OtherNumBytesSent;        // offset 0x10, size 0x4
    unsigned char Padding2[12];            // offset 0x14, size 0xC
    unsigned int NumBytesReceived;         // offset 0x20, size 0x4
    unsigned char Padding3[12];            // offset 0x24, size 0xC
    unsigned int NumBytesSent;             // offset 0x30, size 0x4
    unsigned char Padding4[12];            // offset 0x34, size 0xC
    unsigned char ReceiveBuffer[24576];    // offset 0x40, size 0x6000
    FastPacket SendBuffer;                 // offset 0x6040, size 0x810
    MemoryExchangePacket MemoryExchanger;  // offset 0x6850, size 0x20
    FastPacketBuffer *pOtherSide;          // offset 0x6870, size 0x4
    FastPacket *pReceivePacket;            // offset 0x6874, size 0x4
    FastPacket *pSendPacket;               // offset 0x6878, size 0x4
    unsigned int SendPacketNumber;         // offset 0x687C, size 0x4
    unsigned int ReceivePacketNumber;      // offset 0x6880, size 0x4
    FastPacketFunctionInfo FunctionInfo;   // offset 0x6884, size 0x8
    unsigned char SuspiciousPadding[64];   // offset 0x688C, size 0x40
};

#endif
