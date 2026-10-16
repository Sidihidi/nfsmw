#ifndef MISC_BASE64_H
#define MISC_BASE64_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

// total size: 0x1 (DWARF de GameCube). Encode lo llama OnlineManager::SendEndOfRaceResults en la A124.
struct Base64 {
    Base64() {}
    ~Base64() {}

    static unsigned long Encode(const void *source, const unsigned long source_buflen, void *dest,
                                const unsigned long dest_buflen, const bool terminate);

    static const unsigned char encode_table[64];
    static const unsigned int decode_table[256];
};

#endif
