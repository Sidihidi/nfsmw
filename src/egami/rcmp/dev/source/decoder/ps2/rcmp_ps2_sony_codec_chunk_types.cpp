// rcmp: los trozos del codec MPEG de Sony de PS2 (decoder/ps2/rcmp_ps2_sony_codec_chunk_types.cpp en el .mdebug de la
// A124): la cabecera 'MPCh' y los datos 'MPCd'.
namespace {

const unsigned int ChunkTypes[2] = {0x4D504368, 0x4D504364};

} // namespace

namespace RCMP {

int PS2_SONY_CODEC_is_chunk_for_codec(unsigned int chunktype) {
    unsigned int i;

    for (i = 0; i < 2; i++) {
        if (ChunkTypes[i] == chunktype) {
            return 1;
        }
    }
    return 0;
}

} // namespace RCMP
