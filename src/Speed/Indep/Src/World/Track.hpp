#ifndef WORLD_TRACK_H
#define WORLD_TRACK_H

#include "Speed/Indep/bWare/Inc/bMath.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/bWare/Inc/bChunk.hpp"
#include "Speed/Indep/Src/World/TerrainType.hpp"

// total size: 0x8
class TopologyCoordinate : public bTNode<TopologyCoordinate> {
  public:
    TopologyCoordinate() {}
    ~TopologyCoordinate() {}
    int HasTopology(const bVector2 *position);
    float GetElevation(const bVector3 *position, TerrainType *type, bVector3 *normal, bool *point_valid);
};

enum TopologyTreePolygonFlags {
    TOPOLOGY_TREE_POLYGON_FLAG_OVERLAPPING = 1,
    TOPOLOGY_TREE_POLYGON_FLAG_VERTICAL = 2,
    TOPOLOGY_TREE_POLYGON_FLAG_INFINITELY_HIGH = 4,
    TOPOLOGY_TREE_POLYGON_FLAG_UPSIDE_DOWN = 8,
    TOPOLOGY_TREE_POLYGON_FLAG_4VERTICES = 16,
    TOPOLOGY_TREE_POLYGON_FLAG_USED_BY_NEIGHBOUR = 32,
    TOPOLOGY_TREE_POLYGON_FLAG_OVERLAPPING_ABOVE = 64,
};

// total size: 0x20
struct TopologyTreePolygon {
    void GetVertex(bVector3 *vertex, int vert_num);
    float GetX(int vert_num);
    float GetY(int vert_num);
    void GetVertices(bVector3 *vertices);
    void SetVertex(const bVector3 *vertex, int vert_num);
    void ConvertToLocalVertices(bVector3 *local_vertices, const bVector3 *vertices, bVector2 *square_center);
    void ConvertFromLocalVertices(bVector3 *vertices, const bVector3 *local_vertices, bVector2 *square_center);
    int IsInBBox(int x, int y);
    float GetBBoxMinX();
    float GetBBoxMinY();
    float GetBBoxMaxX();
    float GetBBoxMaxY();
    float GetZBase();
    void SetZBase(float z);
    TerrainType GetTerrainType();
    void SetTerrainType(int terrain_type);
    TopologyTreePolygonFlags GetFlags();
    void SetFlags(int flags);
    int AreThereOverlappingPolygons();
    void SetOverlappingPolygonBit();
    int IsOverlapping();
    void SetOverlapping();
    int IsVertical();
    void SetVertical();
    int IsUpsideDown();
    void SetUpsideDown();
    short GetTrackPolygonNumber();
    void SetTrackPolygonNumber(int number);
    int IsUsedByNeighbour();
    void SetUsedByNeighbourFlag();
    void ClearUsedByNeighbourFlag();
    unsigned char GetGroupNumber();
    void SetGroupNumber(unsigned char group_number);

    unsigned char GroupNumber;        // offset 0x0, size 0x1
    unsigned char MaxMinBits;         // offset 0x1, size 0x1
    unsigned char Type;               // offset 0x2, size 0x1
    unsigned char Flags;              // offset 0x3, size 0x1
    short ZBase;                      // offset 0x4, size 0x2
    short TrackPolygonNumber;         // offset 0x6, size 0x2
    short VerticesX[4];               // offset 0x8, size 0x8
    short VerticesY[4];               // offset 0x10, size 0x8
    short VerticesZ[4];               // offset 0x18, size 0x8
};

struct TrackPolygonCollisionBody;

// total size: 0x74
struct TrackPolygon : public bTNode<TrackPolygon> {
    static void *operator new(unsigned int size);
    static void operator delete(void *ptr);

    TrackPolygon();
    ~TrackPolygon();
    TrackPolygon(TopologyTreePolygon *polygon);

    float GetElevation(bVector2 *point);
    TerrainType GetTerrainType();
    int GetNumVertices();
    bVector3 *GetVertex(int n);
    int GetFlags();
    int AreThereOverlappingPolygons();
    int IsVertical();
    int IsInfinitelyHigh();
    int IsUpsideDown();
    TrackPolygonCollisionBody *GetCollisionBody();
    void SetCollisionBody(TrackPolygonCollisionBody *tpcb);
    int GetNumber();
    const bVector3 *GetNormal();
    bVector2 *GetBBoxMin();
    bVector2 *GetBBoxMax();

    void Close();
    float GetAmountOutside(const bVector2 *point);
    float GetDistanceOutside(const bVector2 *point, float max_distance);
    bool PointInShaft(bVector3 *p, float *pShaftElevation);
    bool SegmentIntersects(const bVector3 *p1, const bVector3 *p2, bVector3 *pIntersection);
    bool TestSegmentToPoly(float *pNearest, const bVector3 *pTarget, const bVector3 *pPos, bVector3 *pMin, bVector3 *pMax, bool bTwoSided,
                           bVector3 *pIntersection);

    char ReferenceCount;                          // offset 0x8, size 0x1
    char NumVertices;                             // offset 0x9, size 0x1
    char Flags;                                   // offset 0xA, size 0x1
    char TheTerrainType;                          // offset 0xB, size 0x1
    TopologyTreePolygon *pPolygon;                // offset 0xC, size 0x4
    TrackPolygonCollisionBody *pCollisionBody;    // offset 0x10, size 0x4
    bVector2 BBoxMin;                             // offset 0x14, size 0x8
    bVector2 BBoxMax;                             // offset 0x1C, size 0x8
    bVector3 Vertices[4];                         // offset 0x24, size 0x40
    float Normal[3];                              // offset 0x64, size 0xC
    float PlaneDistance;                          // offset 0x70, size 0x4
};

// gcdatos11: la familia de formato de fichero de la topologia, ausente del
// arbol. DWARF GC (symbols/mw_dwarfdump.nothpp, CU zTrack/zSpeech) y stabs
// PS2 (symbols/PS2/PS2_types.nothpp) coinciden campo a campo; TopologyTreeFilePolygon
// es 0x9C en GC y 0xA0 en PS2 por el alineado a 16 de bVector3 (sin relleno
// manual, como TrackPolygon). TopologyTree/TopologyTreeBuilder/SceneryGroupInfo
// son opacos en ambos volcados (solo forward, como TrackPolygonCollisionBody).
// Solo declaraciones: ninguna variable de estos tipos sobrevive en los
// volcados (FreeTrackPolygonList incluida: sin direccion en GC ni en A124).
struct TopologyTree;
struct TopologyTreeBuilder;
struct SceneryGroupInfo;

// total size: 0x4
struct TopologyGridEntry {
    unsigned short FirstLookupIndex; // offset 0x0, size 0x2
    unsigned short NumLookups;       // offset 0x2, size 0x2
};

// total size: 0x4
struct TopologyGridLookup {
    unsigned short PolygonNumber; // offset 0x0, size 0x2
    unsigned short CoverageBits;  // offset 0x2, size 0x2
};

// total size: 0x30
struct TopologyTreeHeader {
    short SectionNumber;    // offset 0x0, size 0x2
    char EndianSwapped;     // offset 0x2, size 0x1
    char Padding;           // offset 0x3, size 0x1
    short GridDimensionsX;  // offset 0x4, size 0x2
    short GridDimensionsY;  // offset 0x6, size 0x2
    bVector2 GridMin;       // offset 0x8, size 0x8
    bVector3 ExtentsMin;    // offset 0x10, size 0x10
    bVector3 ExtentsMax;    // offset 0x20, size 0x10

    void EndianSwap();
};

// total size: 0x20
struct TopologyGroupInfo {
    unsigned int NameHash; // offset 0x0, size 0x4
    unsigned int NumPolys; // offset 0x4, size 0x4
    char Enabled;          // offset 0x8, size 0x1
    char Name[23];         // offset 0x9, size 0x17
};

// total size: 0x9C
struct TopologyTreeFilePolygon {
    bool VerifyVertical();
    bVector2 *GetVerticalPos0();
    bVector2 *GetVerticalPos1();

    bVector3 Vertices[4];        // offset 0x0, size 0x40
    TerrainType Type;            // offset 0x40, size 0x4
    unsigned int TextureNameHash; // offset 0x44, size 0x4
    unsigned int GroupNameHash;   // offset 0x48, size 0x4
    int SectionNumber;           // offset 0x4C, size 0x4
    char NumVertices;            // offset 0x50, size 0x1
    char IsVertical;             // offset 0x51, size 0x1
    char IsDoubleSided;          // offset 0x52, size 0x1
    char DoneTroughTest;         // offset 0x53, size 0x1
    unsigned int SortHash;       // offset 0x54, size 0x4
    SceneryGroupInfo *pGroupInfo; // offset 0x58, size 0x4
    char DebugName[32];          // offset 0x5C, size 0x20
    char SourceType[32];         // offset 0x7C, size 0x20
};

// total size: 0x28
struct TrackDataDesc {
    char DebugName[32];   // offset 0x0, size 0x20
    unsigned int Handle;  // offset 0x20, size 0x4
    int TrackNumber;      // offset 0x24, size 0x4
};

// total size: 0x60
class TrackOBB {
  public:
    void EndianSwap() {
        bPlatEndianSwap(&TypeNameHash);
        bPlatEndianSwap(&Matrix);
        bPlatEndianSwap(&Dims);
    }

    unsigned int TypeNameHash; // offset 0x0, size 0x4
    unsigned int Pad[3];       // offset 0x4, size 0xC
    bMatrix4 Matrix;           // offset 0x10, size 0x40
    bVector3 Dims;             // offset 0x50, size 0x10
};

void EstablishRemoteCaffeineConnection();
int GetNumTrackOBBs();
TrackOBB *GetTrackOBB(int index);
int LoaderTrackOBB(bChunk *chunk);
int UnloaderTrackOBB(bChunk *chunk);

#endif
