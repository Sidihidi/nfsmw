#include "Speed/Indep/bWare/Src/PSX2/FastPacketBuffer.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"

#include <eekernel.h>

// indep/bware/src/psx2/FastPacketBuffer.cpp de la A124 (lado del EE). Las cadenas son las del original; el "[EE ]"
// que llevan los avisos es el nombre de este lado del canal.
//
// CanSendPacket y GetSendPacket redondean el tamano a 16 con bAlignUp (bMath.hpp; linea 220 en la A124).

static unsigned int CalculateChecksum(const void *data, int size) {
    unsigned int checksum = 0;

    for (int i = 0; i < size; i++) {
        checksum += static_cast<const unsigned char *>(data)[i];
    }
    return checksum;
}

static void DoNothing() {}

void FastPacketBuffer::StartDma(sceSifDmaData *dma_data, int num_dma) {
    unsigned int id = FunctionInfo.sceSifSetDma(dma_data, num_dma);

    if (id == 0) {
        scePrintf("WARNING:  sceSifSetDma() returned error.\n");
        return;
    }

    int safety_timeout = 1000000;
    while (safety_timeout-- != 0) {
        if (FunctionInfo.sceSifDmaStat(id) < 0) {
            break;
        }
    }
    if (safety_timeout <= 0) {
        scePrintf("WARNING:  sceSifDma stuck.  id = %d\n", id);
    }
}

void FastPacketBuffer::DoDma(void *source_address, void *dest_address, int size) {
    sceSifDmaData dma_data;

    dma_data.data = reinterpret_cast<unsigned int>(source_address);
    dma_data.addr = reinterpret_cast<unsigned int>(dest_address);
    dma_data.size = size;
    dma_data.mode = 0;
    StartDma(&dma_data, 1);
}

void FastPacketBuffer::Reset() {
    bMemSet(this, 0, sizeof(FastPacketBuffer));
    FunctionInfo.sceSifSetDma = sceSifSetDma;
    FunctionInfo.sceSifDmaStat = sceSifDmaStat;
}

void FastPacketBuffer::Start(FastPacketBuffer *other_side) {
    pOtherSide = other_side;
}

void FastPacketBuffer::Stop() {
    pOtherSide = 0;
}

bool FastPacketBuffer::CanReceivePacket() {
    return GetNumBytesToReceive() > 0;
}

int FastPacketBuffer::GetNumBytesToReceive() {
    if (pOtherSide && OtherNumBytesSent != NumBytesReceived) {
        FastPacket *receive_packet = reinterpret_cast<FastPacket *>(&ReceiveBuffer[NumBytesReceived % sizeof(ReceiveBuffer)]);

        if (receive_packet->DataSize == 0) {
            NumBytesReceived += receive_packet->TotalSize;
            ReceivePacketNumber++;
            return GetNumBytesToReceive();
        }
        return receive_packet->DataSize;
    }
    return 0;
}

void *FastPacketBuffer::GetReceivePacket(int *psize) {
    if (!CanReceivePacket()) {
        return 0;
    }
    if (pReceivePacket) {
        scePrintf("WARNING:  %s GetReceivePacket() called without AdvanceReceivePacket()\n", "[EE ]");
    }
    pReceivePacket = reinterpret_cast<FastPacket *>(&ReceiveBuffer[NumBytesReceived % sizeof(ReceiveBuffer)]);

    if (pReceivePacket->PacketNumber != ReceivePacketNumber) {
        scePrintf("WARNING:  %s PacketNumber out of sync.  received %d, expected %d.\n", "[EE ]", pReceivePacket->PacketNumber,
                  ReceivePacketNumber);
        return 0;
    }
    if (pReceivePacket->Checksum) {
        unsigned int checksum = CalculateChecksum(pReceivePacket->Data, pReceivePacket->DataSize);

        if (pReceivePacket->Checksum != checksum) {
            scePrintf("WARNING:  %s Packet checksum 0x%08x wrong, expected 0x%08x\n", "[EE ]", checksum, pReceivePacket->Checksum);
            return 0;
        }
    }
    if (psize) {
        *psize = pReceivePacket->DataSize;
    }
    return pReceivePacket->Data;
}

void FastPacketBuffer::AdvanceReceivePacket() {
    if (!pReceivePacket) {
        scePrintf("WARNING:  %s AdvanceReceivePacket() called without GetReceivePacket()\n", "[EE ]");
    }
    if (pReceivePacket->Checksum) {
        unsigned int checksum = CalculateChecksum(pReceivePacket->Data, pReceivePacket->DataSize);

        if (pReceivePacket->Checksum != checksum) {
            scePrintf("WARNING:  %s Packet checksum 0x%08x wrong, expected 0x%08x [! This is in ::AdvanceReceivePacket()!] \n", "[EE ]",
                      checksum, pReceivePacket->Checksum);
        }
    }
    NumBytesReceived += pReceivePacket->TotalSize;
    ReceivePacketNumber++;
    DoDma(&NumBytesReceived, &pOtherSide->OtherNumBytesReceived, 16);
    pReceivePacket = 0;
}

bool FastPacketBuffer::CanSendPacket(int size) {
    if (size > static_cast<int>(sizeof(SendBuffer.Data))) {
        scePrintf("ERROR:  FastPacketBuffer::GetSendPacket() size = %d is too big, max is %d\n", size, sizeof(SendBuffer.Data));
        return false;
    }
    if (pSendPacket) {
        scePrintf("ERROR:  FastPacketBuffer::GetSendPacket() called but previous packet was not AdvanceReceivePacket()'d.\n");
        return false;
    }
    if (!pOtherSide) {
        scePrintf("ERROR:  FastPacketBuffer::GetSendPacket() called when not initialized.\n");
        return false;
    }

    int total_size = bAlignUp(size, 16) + sizeof(FastPacketHeader);
    int room_to_top = sizeof(ReceiveBuffer) - NumBytesSent % sizeof(ReceiveBuffer);

    total_size += (room_to_top < total_size) * room_to_top;

    int num_free = sizeof(ReceiveBuffer) - (NumBytesSent - OtherNumBytesReceived);
    return num_free >= total_size;
}

void *FastPacketBuffer::GetSendPacket(int size) {
    if (!CanSendPacket(size)) {
        return 0;
    }

    int total_size = bAlignUp(size, 16) + sizeof(FastPacketHeader);
    int room_to_top = sizeof(ReceiveBuffer) - NumBytesSent % sizeof(ReceiveBuffer);

    if (room_to_top < total_size) {
        GetSendPacket(room_to_top - sizeof(FastPacketHeader));
        pSendPacket->DataSize = 0;
        AdvanceSendPacket();
    }

    pSendPacket = &SendBuffer;
    pSendPacket->DataSize = size;
    pSendPacket->TotalSize = total_size;
    return pSendPacket->Data;
}

void FastPacketBuffer::AdvanceSendPacket() {
    if (!pSendPacket) {
        scePrintf("ERROR: FastPacketBuffer::AdvanceSendPacket() called without GetSendPacket()\n");
        return;
    }
    pSendPacket->PacketNumber = SendPacketNumber++;
    pSendPacket->Checksum = 0;
    DoDma(pSendPacket, &pOtherSide->ReceiveBuffer[NumBytesSent % sizeof(ReceiveBuffer)], pSendPacket->TotalSize);
    NumBytesSent += pSendPacket->TotalSize;

    if (pSendPacket->DataSize != 0) {
        DoDma(&NumBytesSent, &pOtherSide->OtherNumBytesSent, 16);
    }
    pSendPacket = 0;
}

unsigned int FastPacketBuffer::ReadMemory(unsigned int address) {
    if (!MemoryExchanger.Done || address < MemoryExchanger.Address || address >= MemoryExchanger.Address + MemoryExchanger.Size) {
        bMemSet(&MemoryExchanger, 0, sizeof(MemoryExchanger));
        MemoryExchanger.DoReadDMA = 1;
        MemoryExchanger.DoWriteDMA = 0;
        MemoryExchanger.Done = 0;
        MemoryExchanger.Address = address & ~15;
        MemoryExchanger.Size = 16;

        DoDma(reinterpret_cast<void *>(address), pOtherSide->MemoryExchanger.Data, 16);
        DoDma(&MemoryExchanger, &pOtherSide->MemoryExchanger, 16);

        while (!MemoryExchanger.Done) {
            DoNothing();
        }
        return ReadMemory(address);
    }
    return *reinterpret_cast<unsigned int *>(&MemoryExchanger.Data[address - MemoryExchanger.Address]);
}

void FastPacketBuffer::WriteMemory(unsigned int address, unsigned int value) {
    ReadMemory(address);
    MemoryExchanger.DoWriteDMA = 1;
    MemoryExchanger.Done = 0;

    *reinterpret_cast<unsigned int *>(&MemoryExchanger.Data[address - MemoryExchanger.Address]) = value;

    DoDma(&MemoryExchanger, &pOtherSide->MemoryExchanger, sizeof(MemoryExchanger));

    while (!MemoryExchanger.Done) {
        DoNothing();
    }
}
