//
//
//
//
//
//
//
//
#ifndef JOYLOG_HPP
#define JOYLOG_HPP // Decl: 10

// Las dos PS2 no tienen JOYLOG_CHANNEL_QUEUEDFILENAME: los stabs de la A124 acaban en JOYLOG_NUM_CHANNELS = 13
// y SLUS pasa 0xA como canal de PRINTF, igual que la A124.
#ifdef EA_PLATFORM_PLAYSTATION2
#define MAX_JOYLOG_CHANNELS 13
#else
#define MAX_JOYLOG_CHANNELS 14 // Decl: 12
#endif

#include <types.h>

// Decl: 15
enum JoylogChannel {
    JOYLOG_CHANNEL_NONE = 0,
    JOYLOG_CHANNEL_CONFIG,
    JOYLOG_CHANNEL_JOYEVENTS,
    JOYLOG_CHANNEL_TIMESTEP,
    JOYLOG_CHANNEL_CHECKSUM,
    JOYLOG_CHANNEL_LOOP_COUNTER,
    JOYLOG_CHANNEL_QUEUEDFILE_STATUS,
#ifndef EA_PLATFORM_PLAYSTATION2
    JOYLOG_CHANNEL_QUEUEDFILENAME,
#endif
    JOYLOG_CHANNEL_MOVIE_PLAYER_STATUS,
    JOYLOG_CHANNEL_MEMORY_CARD,
    JOYLOG_CHANNEL_RANDOM,
    JOYLOG_CHANNEL_PRINTF,
    JOYLOG_CHANNEL_SOUND_LOADING,
    JOYLOG_CHANNEL_PATHFINDER_TIMEOUT,
    JOYLOG_NUM_CHANNELS,
};

class JoylogBuffer;

// total size: 0x1
// Decl: 40
class Joylog {
  public:
    static int IsReplaying();                                                // Decl: 43
    static uint32 GetData(int data_size, JoylogChannel channel_number);      // Decl: 44
    static int32 GetSignedData(int data_size, JoylogChannel channel_number); // Decl: 45
    // Decl: 46
    static float GetData(JoylogChannel channel_number) {
        int data = static_cast<int>(GetData(32, channel_number));
        return *reinterpret_cast<float *>(&data);
    }
    static void GetData(void *data, int data_size_bytes, JoylogChannel channel_number); // Decl: 47

    static uint32 IsCapturing();                                                       // Decl: 50
    static void AddData(int32 data, int data_size_bits, JoylogChannel channel_number); // Decl: 51
    // Decl: 52
    static void AddData(float data, JoylogChannel channel_number) {
        AddData(*reinterpret_cast<int *>(&data), 32, channel_number);
    }
    static void AddData(const void *data, int data_size_bytes, JoylogChannel channel_number); // Decl: 53

    static uint32 AddOrGetData(uint32 data, int data_size, JoylogChannel channel_number);     // Decl: 56
    static int32 AddOrGetSignedData(int32 data, int data_size, JoylogChannel channel_number); // Decl: 57
    static float AddOrGetData(float data, JoylogChannel channel_number);                      // Decl: 58
    static void AddOrGetData(char *string, JoylogChannel channel_number);                     // Decl: 59
    static void AddOrGetData(uint16 *string, JoylogChannel channel_number);                   // Decl: 60

    static void VerifyData(int32 data, int data_size, JoylogChannel channel_number); // Decl: 63

    static void Init(); // Decl: 65
    static void Save(); // Decl: 66

    static void StopReplaying(); // Decl: 68

    static void SetChecksumError(); // Decl: 70

    static void PrintNearbyJoylogEntries(int error_pos); // Decl: 72

    static void Suspend();                 // Decl: 75
    static void Resume();                  // Decl: 76
    static int GetPosition();              // Decl: 77
    static void SetPosition(int position); // Decl: 78

    static void LoadReadAheadBuffer();                                        // Decl: 81
    static void FreeReadAheadBuffer();                                        // Decl: 82
    static int ReadAheadFromChannel(void *buf, int size, int channel_number); // Decl: 83

    // static void RewindReadAheadBuffer() {}

    // static unsigned int IsJuiceReplay() {}

    // static void SetJuiceReplay(int val) {}

  private:
    static int ReplayingFlag;              // size: 0x4, Decl: 91
    static int CapturingFlag;              // size: 0x4, Decl: 92
    static JoylogBuffer *pReplayingBuffer; // size: 0x4, Decl: 93
    static JoylogBuffer *pCapturingBuffer; // size: 0x4, Decl: 94

    static int32 ReadAheadBufferSize; // size: 0x4, Decl: 96
    static int32 ReadAheadBufferPos;  // size: 0x4, Decl: 97

    static uint8 *ReadAheadBuffer; // size: 0x4, Decl: 99
    static int JuiceReplayFlag;    // size: 0x4, Decl: 100
};

void InitJoylog();
void ServiceJoylog();

#endif
