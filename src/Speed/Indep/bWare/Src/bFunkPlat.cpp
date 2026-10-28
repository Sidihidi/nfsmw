#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/bWare/Inc/bFunk.hpp"
#include <types.h>

#ifdef EA_PLATFORM_PLAYSTATION2
#include "Speed/PSX2/bWare/Src/ee/include/eekernel.h"
#endif

bList bFunkServerList;
#ifdef EA_PLATFORM_PLAYSTATION2
int bSonyToolConnected = false;
#endif

// Los ganchos del monitor (bDebug.cpp define los punteros). STRIPPED en GameCube; la A124 y PC los tienen.
extern void (*SendPacketFunction)(struct bFunkPacketHeader *, const void *, int);
extern void (*ServiceMonitorFunction)();
#ifdef EA_PLATFORM_PLAYSTATION2
#include <stdio.h>

// Un digito hexadecimal en mayusculas (bFunkPlat.cpp:32-34 de la A124; el nombre no sale en los stabs).
static inline unsigned char bFunkHexDigit(unsigned char x) {
    if (x < 10) return x + '0';
    if (x < 16) return x + 'A' - 10;
    return '0';
}

// El paquete sale por la consola del kit de desarrollo (DTL-T10000) como texto: "BFUNK:", la posicion y los bytes
// en hexadecimal.
void bSendPacketDTLT10000(struct bFunkPacketHeader *header, const void *data, long data_size) {
    static MUTEX mutex;
    static bool created_mutex = false;
    static int packet_id = 0;
    char DTLT10000PacketBuffer[200];
    int total_data_size;
    int total_data_pos;

    if (!created_mutex) {
        created_mutex = true;
        MUTEX_create(&mutex);
    }
    MUTEX_lock(&mutex);

    header->SourceServer = 0xa280a3c6;
    header->SetChecksumParameters(packet_id, bFunkPacketHeader::CalculateChecksum(data, data_size));
    packet_id++;

    total_data_size = static_cast<int>(data_size) + sizeof(bFunkPacketHeader);
    total_data_pos = 0;
    while (total_data_pos < total_data_size) {
        int text_pos;
        bStrCpy(DTLT10000PacketBuffer, "BFUNK:");
        text_pos = bStrLen(DTLT10000PacketBuffer);
        DTLT10000PacketBuffer[text_pos++] = bFunkHexDigit((total_data_pos >> 12) & 0xf);
        DTLT10000PacketBuffer[text_pos++] = bFunkHexDigit((total_data_pos >> 8) & 0xf);
        DTLT10000PacketBuffer[text_pos++] = bFunkHexDigit((total_data_pos >> 4) & 0xf);
        DTLT10000PacketBuffer[text_pos++] = bFunkHexDigit(total_data_pos & 0xf);
        while (total_data_pos < total_data_size && text_pos < 196) {
            unsigned char c;
            if (total_data_pos < static_cast<int>(sizeof(bFunkPacketHeader))) {
                c = reinterpret_cast<unsigned char *>(header)[total_data_pos];
            } else {
                c = static_cast<const unsigned char *>(data)[total_data_pos - sizeof(bFunkPacketHeader)];
            }
            DTLT10000PacketBuffer[text_pos++] = bFunkHexDigit(c >> 4);
            DTLT10000PacketBuffer[text_pos++] = bFunkHexDigit(c & 0xf);
            total_data_pos++;
        }
        DTLT10000PacketBuffer[text_pos] = 0;
        puts(DTLT10000PacketBuffer);
    }
    MUTEX_unlock(&mutex);
}
#endif

void bSetupMonitorFunctionHooks(void (*f2)(struct bFunkPacketHeader *, const void *, int), void (*f3)()) {
    SendPacketFunction = f2;
    ServiceMonitorFunction = f3;
}

void bSendPacket(struct bFunkPacketHeader *header, const void *data, int size) {
    if (SendPacketFunction) {
        SendPacketFunction(header, data, size);
        return;
    }
#ifdef EA_PLATFORM_PLAYSTATION2
    if (bSonyToolConnected) {
        bSendPacketDTLT10000(header, data, size);
    }
#endif
}

void bMonitorService() {
    if (ServiceMonitorFunction) {
        ServiceMonitorFunction();
    }
}

// STRIPPED
uint16 bFunkPacketHeader::CalculateChecksum(const void *data, int data_size) {
    uint16 checksum = 0;
    for (int i = 0; i < data_size; i++) {
        checksum += static_cast<const uint8 *>(data)[i];
    }
    return checksum;
}

#ifdef EA_PLATFORM_PLAYSTATION2
void bFunkCallASync(const char *server_name, int function_num, const void *data, int size) {
    bFunkPacketHeader header;

    if (bIsCodeineConnected() && size <= 2016) {
        bMemSet(&header, 0, sizeof(header));
        header.SetDataSize(size);
        header.Type = 0;
        header.FunctionNum = function_num;
        header.DestServer = bStringHash(server_name);
        bMonitorService();
        bSendPacket(&header, data, size);
    }
}
#else
void bFunkCallASync(const char *server_name, int function_num, const void *data, int size) {
    bFunkPacketHeader header;
}
#endif

#ifdef EA_PLATFORM_PLAYSTATION2
int bFunkCallSync(const char *server_name, int function_num, const void *data, int size, void *return_data, int max_return_size) {
    volatile int return_code;
    bFunkPacketHeader header;

    if (bIsCodeineConnected()) {
        if (size > 2016 || max_return_size > 2016) {
            return -6;
        }
        return_code = -10;
        bMemSet(&header, 0, sizeof(header));
        header.SetDataSize(size);
        header.Type = 1;
        header.FunctionNum = function_num;
        header.DestServer = bStringHash(server_name);
        header.ReturnBufferAddress = reinterpret_cast<uint32>(return_data);
        header.ReturnCodeAddress = reinterpret_cast<uint32>(&return_code);
        header.MaxReturnSize = max_return_size;
        bSendPacket(&header, data, size);
        while (return_code == -10) {
            bMonitorService();
        }
        return return_code;
    }
    return -5;
}
#else
int bFunkCallSync(const char *server_name, int function_num, const void *data, int size, void *return_data, int max_return_size) {
    volatile int return_code;
    bFunkPacketHeader header;

    return -5;
}
#endif

// STRIPPED
int bFunkCallSyncTimeout(const char *server_name, int function_num, const void *data, int size, void *return_data, int timeout_ms,
                         int max_return_size) {}

int bFunkDoesServerExist(const char *server_name) {
    uint32 server_hash = bStringHash(server_name);
    int result;
    int return_code = bFunkCallSync("CODEINE", 0x16, server_name, bStrLen(server_name) + 1, &result, 4);

    if (return_code == 4) {
        return result;
    }

    return 0;
}

// STRIPPED
void bNewSymbolFile(const char *filename) {}

void bRefreshTweaker() {
    bFunkCallASync("CODEINE", 0x28, nullptr, 0);
}

// STRIPPED
int bGetFunctionInfo(unsigned int address, unsigned int *start_address, char *name, int max_sizeof_name) {}

// STRIPPED
bFunkServer::bFunkServer(const char *name) {}

bFunkServer::~bFunkServer() {
    reinterpret_cast<bNode *>(this)->Remove();
}

// STRIPPED
void bFunkServer::AddASync(int function_num, void (*function)(const void *, int)) {}

// STRIPPED
void bFunkServer::AddSync(int function_num, int (*function)(const void *, int, void *)) {}

// STRIPPED
void bFunkServer::Remove(int function_num) {}

#ifdef EA_PLATFORM_PLAYSTATION2
void bFunkServer::ProcessPacket(const bFunkPacket *packet) {
    if (packet->GetType() == 0) {
        int function_num = packet->FunctionNum;

        if (FunctionTypes[function_num] == 0) {
            bFunkHandleASyncFunction *func = (bFunkHandleASyncFunction *)FunctionTable[function_num];
            if (func) {
                func(packet->GetData(), packet->GetDataSize());
                return;
            }
        }
        if (FunctionTypes[function_num] == 1) {
            bFunkHandleSyncFunction *func = (bFunkHandleSyncFunction *)FunctionTable[function_num];
            if (func) {
                func(packet->GetData(), packet->GetDataSize(), nullptr);
            }
        }
    } else if (packet->GetType() == 1) {
        bFunkPacket return_packet;
        int return_code = -4;
        void *func = FunctionTable[packet->FunctionNum];

        if (func) {
            if (FunctionTypes[packet->FunctionNum] == 0) {
                ((bFunkHandleASyncFunction *)func)(packet->GetData(), packet->GetDataSize());
                return_code = 0;
            } else {
                return_code = ((bFunkHandleSyncFunction *)func)(packet->GetData(), packet->GetDataSize(), return_packet.GetData());
            }
        }
        return_packet.InitReturnPacketHeader(packet, return_code);
        DeliverPacket(&return_packet);
    } else if (packet->GetType() == 2) {
        if (packet->ReturnCode > 0) {
            bMemCpy((void *)packet->ReturnBufferAddress, packet->GetData(), packet->ReturnCode);
        }
        *(int *)packet->ReturnCodeAddress = packet->ReturnCode;
    }
}
#else
// TODO
void bFunkServer::ProcessPacket(const bFunkPacket *packet) {}
#endif

struct bFunkServer *bFunkFindServer(uint32 server_hash) {
    for (bFunkServer *server = static_cast<bFunkServer *>(bFunkServerList.GetHead()); server != bFunkServerList.EndOfList();
         server = server->GetNext()) {
        if (server->GetNameHash() == server_hash) {
            return server;
        }
    }
    return nullptr;
}

// TODO
bool bFunkServer::CanDeliverPacket(uint32 server_hash) {
    if (server_hash == NameHash || bFunkFindServer(server_hash) != nullptr) {
        return true;
    }
    return false;
}

#ifdef EA_PLATFORM_PLAYSTATION2
bool bFunkServer::DeliverPacket(bFunkPacket *packet) {
    if (packet->DestServer == NameHash) {
        ProcessPacket(packet);
        return true;
    } else {
        bFunkServer *server = bFunkFindServer(packet->DestServer);
        if (server) {
            server->ProcessPacket(packet);
            return true;
        }
    }
    return false;
}
#else
// TODO
bool bFunkServer::DeliverPacket(bFunkPacket *packet) {}
#endif

// STRIPPED
bFunkServerPlatform::bFunkServerPlatform(const char *name, int max_receive_packets) : bFunkServer(name) {}

#ifdef EA_PLATFORM_PLAYSTATION2
bFunkServerPlatform::~bFunkServerPlatform() {
    bFunkCallASync("CODEINE", 0x15, GetName(), bStrLen(GetName()) + 1);
}
#else
// STRIPPED
bFunkServerPlatform::~bFunkServerPlatform() {}
#endif

#ifdef EA_PLATFORM_PLAYSTATION2
int bFunkServerPlatform::Service() {
    int num_processed = 0;

    if (!ProcessingPacket) {
        bFunkPacket *packet;
        while ((packet = &pReceivePackets[NextReceivePacketNum % MaxReceivePackets])->GetTotalSize()) {
            NextReceivePacketNum++;
            ProcessingPacket = true;
            ProcessPacket(packet);
            num_processed++;
            packet->SetTotalSize(0);
            ProcessingPacket = false;
        }
    }
    return num_processed;
}
#else
// TODO
int bFunkServerPlatform::Service() {}
#endif

#ifdef EA_PLATFORM_PLAYSTATION2
bool bFunkServerPlatform::DeliverPacket(bFunkPacket *packet) {
    if (bIsCodeineConnected()) {
        if (bFunkServer::DeliverPacket(packet)) {
            return true;
        }
        bSendPacket(packet, packet->GetData(), packet->GetDataSize());
        return true;
    }
    return false;
}
#else
// TODO
bool bFunkServerPlatform::DeliverPacket(bFunkPacket *packet) {}
#endif

#ifdef EA_PLATFORM_PLAYSTATION2
void bFunkService() {
    for (bFunkServer *server = static_cast<bFunkServer *>(bFunkServerList.GetHead()); server != bFunkServerList.EndOfList();
         server = server->GetNext()) {
        server->Service();
    }
}
#else
// TODO
void bFunkService() {}
#endif

#ifdef EA_PLATFORM_PLAYSTATION2
void bFigureOutPSX2Platform() {
    bSonyToolConnected = false;
    if (GetMemorySize() > 0x4000000) {
        bSonyToolConnected = true;
    }
}
#endif

#ifdef EA_PLATFORM_GAMECUBE
// STRIPPED
int bIsPlatGameCubeHW2() {}

// STRIPPED
int bIsPlatGameCubeEmulator() {}
#endif
