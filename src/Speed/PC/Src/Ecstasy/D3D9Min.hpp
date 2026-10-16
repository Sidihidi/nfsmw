#ifndef PC_ECSTASY_D3D9MIN_HPP
#define PC_ECSTASY_D3D9MIN_HPP

// Lo minimo de Direct3D 9 que usa la capa grafica de PC, declarado a mano para no copiar cabeceras de Microsoft al
// arbol. El original incluia <d3d9.h> del SDK de DirectX de febrero de 2005 (d3dx9_26.dll). Las interfaces llevan
// TODOS sus metodos en el orden documentado de d3d9.h, porque el juego los llama por la vtable (el desplazamiento de
// cada `call [vtable+N]` es el indice del metodo); los tipos que el juego no toca van solo declarados.

#ifndef EA_PLATFORM_WIN32
#error "D3D9Min.hpp solo es de PC"
#endif

typedef long HRESULT;
typedef unsigned long DWORD;
typedef unsigned int UINT;
typedef int BOOL;
typedef void *HANDLE;
typedef struct HWND__ *HWND;
typedef struct HDC__ *HDC;
typedef struct HMONITOR__ *HMONITOR;
typedef struct _GUID IID;
typedef const IID &REFIID;
typedef struct tagRECT RECT;
typedef struct tagPOINT POINT;
typedef struct _RGNDATA RGNDATA;
typedef struct tagPALETTEENTRY PALETTEENTRY;

typedef DWORD D3DCOLOR;

enum D3DFORMAT {
    D3DFMT_UNKNOWN = 0,
    D3DFMT_A8R8G8B8 = 21,
    D3DFMT_X8R8G8B8 = 22,
    D3DFMT_P8 = 41,
    D3DFMT_INDEX16 = 101,
    D3DFMT_FORCE_DWORD = 0x7fffffff,
};

enum D3DPOOL {
    D3DPOOL_DEFAULT = 0,
    D3DPOOL_MANAGED = 1,
    D3DPOOL_SYSTEMMEM = 2,
    D3DPOOL_SCRATCH = 3,
    D3DPOOL_FORCE_DWORD = 0x7fffffff,
};

#define D3DUSAGE_WRITEONLY (0x00000008L)

// Los resultados de D3D9 que mira el cuadro de PC (eDisplayFrame).
#define D3D_OK 0
#define D3DERR_DEVICELOST ((HRESULT)0x88760868L)
#define D3DERR_DEVICENOTRESET ((HRESULT)0x88760869L)

// Los enumerados que el juego no usa por valor desde estas fuentes van como DWORD: ocupan lo mismo en la pila.
typedef DWORD D3DDEVTYPE;
typedef DWORD D3DRESOURCETYPE;
typedef DWORD D3DMULTISAMPLE_TYPE;
typedef DWORD D3DBACKBUFFER_TYPE;
typedef DWORD D3DTEXTUREFILTERTYPE;
typedef DWORD D3DTRANSFORMSTATETYPE;
typedef DWORD D3DRENDERSTATETYPE;
typedef DWORD D3DSTATEBLOCKTYPE;
typedef DWORD D3DTEXTURESTAGESTATETYPE;
typedef DWORD D3DSAMPLERSTATETYPE;
typedef DWORD D3DPRIMITIVETYPE;
typedef DWORD D3DQUERYTYPE;
typedef DWORD D3DCUBEMAP_FACES;

// d3d9caps.h: las capacidades del dispositivo (0x130 bytes). Solo se nombran los campos que leen estas fuentes.
struct D3DCAPS9 {
    DWORD Unknown0[16];          // offset 0x0 (DeviceType ... TextureCaps)
    DWORD TextureFilterCaps;     // offset 0x40
    DWORD Unknown44[10];         // offset 0x44 (CubeTextureFilterCaps ... MaxTextureAspectRatio)
    DWORD MaxAnisotropy;         // offset 0x6C
    DWORD Unknown70[9];          // offset 0x70 (MaxVertexW ... TextureOpCaps)
    DWORD MaxTextureBlendStages; // offset 0x94
    DWORD Unknown98[38];         // offset 0x98
};
struct D3DDISPLAYMODE;
struct D3DDEVICE_CREATION_PARAMETERS;
struct D3DPRESENT_PARAMETERS;
struct D3DRASTER_STATUS;
struct D3DGAMMARAMP;
// d3d9types.h: el viewport (24 bytes).
struct D3DVIEWPORT9 {
    DWORD X;
    DWORD Y;
    DWORD Width;
    DWORD Height;
    float MinZ;
    float MaxZ;
};
// d3d9types.h: el color de punto flotante, el vector, la luz y el material del pipeline fijo (los usa la luz de los
// coches cuando la tecnica del CarShader no lleva shaders: eSetLightContext/eSetLightMaterial de EcstasyE.cpp).
struct D3DCOLORVALUE {
    float r;
    float g;
    float b;
    float a;
};
struct D3DVECTOR {
    float x;
    float y;
    float z;
};
struct D3DMATERIAL9 {
    D3DCOLORVALUE Diffuse;
    D3DCOLORVALUE Ambient;
    D3DCOLORVALUE Specular;
    D3DCOLORVALUE Emissive;
    float Power;
};
struct D3DLIGHT9 {
    int Type; // D3DLIGHTTYPE (3: D3DLIGHT_DIRECTIONAL)
    D3DCOLORVALUE Diffuse;
    D3DCOLORVALUE Specular;
    D3DCOLORVALUE Ambient;
    D3DVECTOR Position;
    D3DVECTOR Direction;
    float Range;
    float Falloff;
    float Attenuation0;
    float Attenuation1;
    float Attenuation2;
    float Theta;
    float Phi;
};
struct D3DCLIPSTATUS9;
struct D3DRECT;
struct D3DMATRIX;
// d3d9types.h: un elemento de una declaracion de vertices (8 bytes).
struct D3DVERTEXELEMENT9 {
    unsigned short Stream;
    unsigned short Offset;
    unsigned char Type;
    unsigned char Method;
    unsigned char Usage;
    unsigned char UsageIndex;
};
struct D3DRECTPATCH_INFO;
struct D3DTRIPATCH_INFO;
struct D3DSURFACE_DESC;
struct D3DVERTEXBUFFER_DESC;
struct D3DINDEXBUFFER_DESC;
struct D3DLOCKED_RECT;

struct IDirect3D9;
struct IDirect3DDevice9;
struct IDirect3DSwapChain9;
struct IDirect3DSurface9;
struct IDirect3DVolumeTexture9;
struct IDirect3DCubeTexture9;
struct IDirect3DBaseTexture9;
struct IDirect3DStateBlock9;
struct IDirect3DVertexDeclaration9;
struct IDirect3DVertexShader9;
struct IDirect3DPixelShader9;
struct IDirect3DQuery9;

struct IDirect3DResource9 {
    virtual HRESULT __stdcall QueryInterface(REFIID riid, void **ppvObj) = 0;
    virtual unsigned long __stdcall AddRef() = 0;
    virtual unsigned long __stdcall Release() = 0;
    virtual HRESULT __stdcall GetDevice(IDirect3DDevice9 **ppDevice) = 0;
    virtual HRESULT __stdcall SetPrivateData(const IID &refguid, const void *pData, DWORD SizeOfData, DWORD Flags) = 0;
    virtual HRESULT __stdcall GetPrivateData(const IID &refguid, void *pData, DWORD *pSizeOfData) = 0;
    virtual HRESULT __stdcall FreePrivateData(const IID &refguid) = 0;
    virtual DWORD __stdcall SetPriority(DWORD PriorityNew) = 0;
    virtual DWORD __stdcall GetPriority() = 0;
    virtual void __stdcall PreLoad() = 0;
    virtual D3DRESOURCETYPE __stdcall GetType() = 0;
};

struct IDirect3DVertexBuffer9 : public IDirect3DResource9 {
    virtual HRESULT __stdcall Lock(UINT OffsetToLock, UINT SizeToLock, void **ppbData, DWORD Flags) = 0;
    virtual HRESULT __stdcall Unlock() = 0;
    virtual HRESULT __stdcall GetDesc(D3DVERTEXBUFFER_DESC *pDesc) = 0;
};

struct IDirect3DIndexBuffer9 : public IDirect3DResource9 {
    virtual HRESULT __stdcall Lock(UINT OffsetToLock, UINT SizeToLock, void **ppbData, DWORD Flags) = 0;
    virtual HRESULT __stdcall Unlock() = 0;
    virtual HRESULT __stdcall GetDesc(D3DINDEXBUFFER_DESC *pDesc) = 0;
};

struct IDirect3DBaseTexture9 : public IDirect3DResource9 {
    virtual DWORD __stdcall SetLOD(DWORD LODNew) = 0;
    virtual DWORD __stdcall GetLOD() = 0;
    virtual DWORD __stdcall GetLevelCount() = 0;
    virtual HRESULT __stdcall SetAutoGenFilterType(D3DTEXTUREFILTERTYPE FilterType) = 0;
    virtual D3DTEXTUREFILTERTYPE __stdcall GetAutoGenFilterType() = 0;
    virtual void __stdcall GenerateMipSubLevels() = 0;
};

struct IDirect3DTexture9 : public IDirect3DBaseTexture9 {
    virtual HRESULT __stdcall GetLevelDesc(UINT Level, D3DSURFACE_DESC *pDesc) = 0;
    virtual HRESULT __stdcall GetSurfaceLevel(UINT Level, IDirect3DSurface9 **ppSurfaceLevel) = 0;
    virtual HRESULT __stdcall LockRect(UINT Level, D3DLOCKED_RECT *pLockedRect, const RECT *pRect, DWORD Flags) = 0;
    virtual HRESULT __stdcall UnlockRect(UINT Level) = 0;
    virtual HRESULT __stdcall AddDirtyRect(const RECT *pDirtyRect) = 0;
};

struct D3DLOCKED_RECT {
    int Pitch;
    void *pBits;
};

// d3d9types.h: la descripcion de una superficie (32 bytes).
struct D3DSURFACE_DESC {
    D3DFORMAT Format;
    D3DRESOURCETYPE Type;
    DWORD Usage;
    D3DPOOL Pool;
    D3DMULTISAMPLE_TYPE MultiSampleType;
    DWORD MultiSampleQuality;
    UINT Width;
    UINT Height;
};

struct IDirect3DVertexDeclaration9 {
    virtual HRESULT __stdcall QueryInterface(REFIID riid, void **ppvObj) = 0;
    virtual unsigned long __stdcall AddRef() = 0;
    virtual unsigned long __stdcall Release() = 0;
};

struct IDirect3DSurface9 : public IDirect3DResource9 {
    virtual HRESULT __stdcall GetContainer(const IID &riid, void **ppContainer) = 0;
    virtual HRESULT __stdcall GetDesc(D3DSURFACE_DESC *pDesc) = 0;
    virtual HRESULT __stdcall LockRect(D3DLOCKED_RECT *pLockedRect, const RECT *pRect, DWORD Flags) = 0;
    virtual HRESULT __stdcall UnlockRect() = 0;
};

struct IDirect3DCubeTexture9 : public IDirect3DBaseTexture9 {
    virtual HRESULT __stdcall GetLevelDesc(UINT Level, D3DSURFACE_DESC *pDesc) = 0;
    virtual HRESULT __stdcall GetCubeMapSurface(D3DCUBEMAP_FACES FaceType, UINT Level,
                                                IDirect3DSurface9 **ppCubeMapSurface) = 0;
    virtual HRESULT __stdcall LockRect(D3DCUBEMAP_FACES FaceType, UINT Level, D3DLOCKED_RECT *pLockedRect,
                                       const RECT *pRect, DWORD Flags) = 0;
    virtual HRESULT __stdcall UnlockRect(D3DCUBEMAP_FACES FaceType, UINT Level) = 0;
    virtual HRESULT __stdcall AddDirtyRect(D3DCUBEMAP_FACES FaceType, const RECT *pDirtyRect) = 0;
};

struct IDirect3DDevice9 {
    virtual HRESULT __stdcall QueryInterface(REFIID riid, void **ppvObj) = 0;
    virtual unsigned long __stdcall AddRef() = 0;
    virtual unsigned long __stdcall Release() = 0;
    virtual HRESULT __stdcall TestCooperativeLevel() = 0;
    virtual UINT __stdcall GetAvailableTextureMem() = 0;
    virtual HRESULT __stdcall EvictManagedResources() = 0;
    virtual HRESULT __stdcall GetDirect3D(IDirect3D9 **ppD3D9) = 0;
    virtual HRESULT __stdcall GetDeviceCaps(D3DCAPS9 *pCaps) = 0;
    virtual HRESULT __stdcall GetDisplayMode(UINT iSwapChain, D3DDISPLAYMODE *pMode) = 0;
    virtual HRESULT __stdcall GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS *pParameters) = 0;
    virtual HRESULT __stdcall SetCursorProperties(UINT XHotSpot, UINT YHotSpot, IDirect3DSurface9 *pCursorBitmap) = 0;
    virtual void __stdcall SetCursorPosition(int X, int Y, DWORD Flags) = 0;
    virtual BOOL __stdcall ShowCursor(BOOL bShow) = 0;
    virtual HRESULT __stdcall CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS *pPresentationParameters,
                                                        IDirect3DSwapChain9 **pSwapChain) = 0;
    virtual HRESULT __stdcall GetSwapChain(UINT iSwapChain, IDirect3DSwapChain9 **pSwapChain) = 0;
    virtual UINT __stdcall GetNumberOfSwapChains() = 0;
    virtual HRESULT __stdcall Reset(D3DPRESENT_PARAMETERS *pPresentationParameters) = 0;
    virtual HRESULT __stdcall Present(const RECT *pSourceRect, const RECT *pDestRect, HWND hDestWindowOverride,
                                      const RGNDATA *pDirtyRegion) = 0;
    virtual HRESULT __stdcall GetBackBuffer(UINT iSwapChain, UINT iBackBuffer, D3DBACKBUFFER_TYPE Type,
                                            IDirect3DSurface9 **ppBackBuffer) = 0;
    virtual HRESULT __stdcall GetRasterStatus(UINT iSwapChain, D3DRASTER_STATUS *pRasterStatus) = 0;
    virtual HRESULT __stdcall SetDialogBoxMode(BOOL bEnableDialogs) = 0;
    virtual void __stdcall SetGammaRamp(UINT iSwapChain, DWORD Flags, const D3DGAMMARAMP *pRamp) = 0;
    virtual void __stdcall GetGammaRamp(UINT iSwapChain, D3DGAMMARAMP *pRamp) = 0;
    virtual HRESULT __stdcall CreateTexture(UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool,
                                            IDirect3DTexture9 **ppTexture, HANDLE *pSharedHandle) = 0;
    virtual HRESULT __stdcall CreateVolumeTexture(UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage,
                                                  D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture9 **ppVolumeTexture,
                                                  HANDLE *pSharedHandle) = 0;
    virtual HRESULT __stdcall CreateCubeTexture(UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool,
                                                IDirect3DCubeTexture9 **ppCubeTexture, HANDLE *pSharedHandle) = 0;
    virtual HRESULT __stdcall CreateVertexBuffer(UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool,
                                                 IDirect3DVertexBuffer9 **ppVertexBuffer, HANDLE *pSharedHandle) = 0;
    virtual HRESULT __stdcall CreateIndexBuffer(UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool,
                                                IDirect3DIndexBuffer9 **ppIndexBuffer, HANDLE *pSharedHandle) = 0;
    virtual HRESULT __stdcall CreateRenderTarget(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample,
                                                 DWORD MultisampleQuality, BOOL Lockable, IDirect3DSurface9 **ppSurface,
                                                 HANDLE *pSharedHandle) = 0;
    virtual HRESULT __stdcall CreateDepthStencilSurface(UINT Width, UINT Height, D3DFORMAT Format,
                                                        D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality,
                                                        BOOL Discard, IDirect3DSurface9 **ppSurface, HANDLE *pSharedHandle) = 0;
    virtual HRESULT __stdcall UpdateSurface(IDirect3DSurface9 *pSourceSurface, const RECT *pSourceRect,
                                            IDirect3DSurface9 *pDestinationSurface, const POINT *pDestPoint) = 0;
    virtual HRESULT __stdcall UpdateTexture(IDirect3DBaseTexture9 *pSourceTexture, IDirect3DBaseTexture9 *pDestinationTexture) = 0;
    virtual HRESULT __stdcall GetRenderTargetData(IDirect3DSurface9 *pRenderTarget, IDirect3DSurface9 *pDestSurface) = 0;
    virtual HRESULT __stdcall GetFrontBufferData(UINT iSwapChain, IDirect3DSurface9 *pDestSurface) = 0;
    virtual HRESULT __stdcall StretchRect(IDirect3DSurface9 *pSourceSurface, const RECT *pSourceRect,
                                          IDirect3DSurface9 *pDestSurface, const RECT *pDestRect,
                                          D3DTEXTUREFILTERTYPE Filter) = 0;
    virtual HRESULT __stdcall ColorFill(IDirect3DSurface9 *pSurface, const RECT *pRect, D3DCOLOR color) = 0;
    virtual HRESULT __stdcall CreateOffscreenPlainSurface(UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool,
                                                          IDirect3DSurface9 **ppSurface, HANDLE *pSharedHandle) = 0;
    virtual HRESULT __stdcall SetRenderTarget(DWORD RenderTargetIndex, IDirect3DSurface9 *pRenderTarget) = 0;
    virtual HRESULT __stdcall GetRenderTarget(DWORD RenderTargetIndex, IDirect3DSurface9 **ppRenderTarget) = 0;
    virtual HRESULT __stdcall SetDepthStencilSurface(IDirect3DSurface9 *pNewZStencil) = 0;
    virtual HRESULT __stdcall GetDepthStencilSurface(IDirect3DSurface9 **ppZStencilSurface) = 0;
    virtual HRESULT __stdcall BeginScene() = 0;
    virtual HRESULT __stdcall EndScene() = 0;
    virtual HRESULT __stdcall Clear(DWORD Count, const D3DRECT *pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil) = 0;
    virtual HRESULT __stdcall SetTransform(D3DTRANSFORMSTATETYPE State, const D3DMATRIX *pMatrix) = 0;
    virtual HRESULT __stdcall GetTransform(D3DTRANSFORMSTATETYPE State, D3DMATRIX *pMatrix) = 0;
    virtual HRESULT __stdcall MultiplyTransform(D3DTRANSFORMSTATETYPE State, const D3DMATRIX *pMatrix) = 0;
    virtual HRESULT __stdcall SetViewport(const D3DVIEWPORT9 *pViewport) = 0;
    virtual HRESULT __stdcall GetViewport(D3DVIEWPORT9 *pViewport) = 0;
    virtual HRESULT __stdcall SetMaterial(const D3DMATERIAL9 *pMaterial) = 0;
    virtual HRESULT __stdcall GetMaterial(D3DMATERIAL9 *pMaterial) = 0;
    virtual HRESULT __stdcall SetLight(DWORD Index, const D3DLIGHT9 *pLight) = 0;
    virtual HRESULT __stdcall GetLight(DWORD Index, D3DLIGHT9 *pLight) = 0;
    virtual HRESULT __stdcall LightEnable(DWORD Index, BOOL Enable) = 0;
    virtual HRESULT __stdcall GetLightEnable(DWORD Index, BOOL *pEnable) = 0;
    virtual HRESULT __stdcall SetClipPlane(DWORD Index, const float *pPlane) = 0;
    virtual HRESULT __stdcall GetClipPlane(DWORD Index, float *pPlane) = 0;
    virtual HRESULT __stdcall SetRenderState(D3DRENDERSTATETYPE State, DWORD Value) = 0;
    virtual HRESULT __stdcall GetRenderState(D3DRENDERSTATETYPE State, DWORD *pValue) = 0;
    virtual HRESULT __stdcall CreateStateBlock(D3DSTATEBLOCKTYPE Type, IDirect3DStateBlock9 **ppSB) = 0;
    virtual HRESULT __stdcall BeginStateBlock() = 0;
    virtual HRESULT __stdcall EndStateBlock(IDirect3DStateBlock9 **ppSB) = 0;
    virtual HRESULT __stdcall SetClipStatus(const D3DCLIPSTATUS9 *pClipStatus) = 0;
    virtual HRESULT __stdcall GetClipStatus(D3DCLIPSTATUS9 *pClipStatus) = 0;
    virtual HRESULT __stdcall GetTexture(DWORD Stage, IDirect3DBaseTexture9 **ppTexture) = 0;
    virtual HRESULT __stdcall SetTexture(DWORD Stage, IDirect3DBaseTexture9 *pTexture) = 0;
    virtual HRESULT __stdcall GetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD *pValue) = 0;
    virtual HRESULT __stdcall SetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value) = 0;
    virtual HRESULT __stdcall GetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD *pValue) = 0;
    virtual HRESULT __stdcall SetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value) = 0;
    virtual HRESULT __stdcall ValidateDevice(DWORD *pNumPasses) = 0;
    virtual HRESULT __stdcall SetPaletteEntries(UINT PaletteNumber, const PALETTEENTRY *pEntries) = 0;
    virtual HRESULT __stdcall GetPaletteEntries(UINT PaletteNumber, PALETTEENTRY *pEntries) = 0;
    virtual HRESULT __stdcall SetCurrentTexturePalette(UINT PaletteNumber) = 0;
    virtual HRESULT __stdcall GetCurrentTexturePalette(UINT *PaletteNumber) = 0;
    virtual HRESULT __stdcall SetScissorRect(const RECT *pRect) = 0;
    virtual HRESULT __stdcall GetScissorRect(RECT *pRect) = 0;
    virtual HRESULT __stdcall SetSoftwareVertexProcessing(BOOL bSoftware) = 0;
    virtual BOOL __stdcall GetSoftwareVertexProcessing() = 0;
    virtual HRESULT __stdcall SetNPatchMode(float nSegments) = 0;
    virtual float __stdcall GetNPatchMode() = 0;
    virtual HRESULT __stdcall DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount) = 0;
    virtual HRESULT __stdcall DrawIndexedPrimitive(D3DPRIMITIVETYPE Type, int BaseVertexIndex, UINT MinVertexIndex,
                                                   UINT NumVertices, UINT startIndex, UINT primCount) = 0;
    virtual HRESULT __stdcall DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount,
                                              const void *pVertexStreamZeroData, UINT VertexStreamZeroStride) = 0;
    virtual HRESULT __stdcall DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices,
                                                     UINT PrimitiveCount, const void *pIndexData, D3DFORMAT IndexDataFormat,
                                                     const void *pVertexStreamZeroData, UINT VertexStreamZeroStride) = 0;
    virtual HRESULT __stdcall ProcessVertices(UINT SrcStartIndex, UINT DestIndex, UINT VertexCount,
                                              IDirect3DVertexBuffer9 *pDestBuffer, IDirect3DVertexDeclaration9 *pVertexDecl,
                                              DWORD Flags) = 0;
    virtual HRESULT __stdcall CreateVertexDeclaration(const D3DVERTEXELEMENT9 *pVertexElements,
                                                      IDirect3DVertexDeclaration9 **ppDecl) = 0;
    virtual HRESULT __stdcall SetVertexDeclaration(IDirect3DVertexDeclaration9 *pDecl) = 0;
    virtual HRESULT __stdcall GetVertexDeclaration(IDirect3DVertexDeclaration9 **ppDecl) = 0;
    virtual HRESULT __stdcall SetFVF(DWORD FVF) = 0;
    virtual HRESULT __stdcall GetFVF(DWORD *pFVF) = 0;
    virtual HRESULT __stdcall CreateVertexShader(const DWORD *pFunction, IDirect3DVertexShader9 **ppShader) = 0;
    virtual HRESULT __stdcall SetVertexShader(IDirect3DVertexShader9 *pShader) = 0;
    virtual HRESULT __stdcall GetVertexShader(IDirect3DVertexShader9 **ppShader) = 0;
    virtual HRESULT __stdcall SetVertexShaderConstantF(UINT StartRegister, const float *pConstantData, UINT Vector4fCount) = 0;
    virtual HRESULT __stdcall GetVertexShaderConstantF(UINT StartRegister, float *pConstantData, UINT Vector4fCount) = 0;
    virtual HRESULT __stdcall SetVertexShaderConstantI(UINT StartRegister, const int *pConstantData, UINT Vector4iCount) = 0;
    virtual HRESULT __stdcall GetVertexShaderConstantI(UINT StartRegister, int *pConstantData, UINT Vector4iCount) = 0;
    virtual HRESULT __stdcall SetVertexShaderConstantB(UINT StartRegister, const BOOL *pConstantData, UINT BoolCount) = 0;
    virtual HRESULT __stdcall GetVertexShaderConstantB(UINT StartRegister, BOOL *pConstantData, UINT BoolCount) = 0;
    virtual HRESULT __stdcall SetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer9 *pStreamData, UINT OffsetInBytes,
                                              UINT Stride) = 0;
    virtual HRESULT __stdcall GetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer9 **ppStreamData, UINT *pOffsetInBytes,
                                              UINT *pStride) = 0;
    virtual HRESULT __stdcall SetStreamSourceFreq(UINT StreamNumber, UINT Setting) = 0;
    virtual HRESULT __stdcall GetStreamSourceFreq(UINT StreamNumber, UINT *pSetting) = 0;
    virtual HRESULT __stdcall SetIndices(IDirect3DIndexBuffer9 *pIndexData) = 0;
    virtual HRESULT __stdcall GetIndices(IDirect3DIndexBuffer9 **ppIndexData) = 0;
    virtual HRESULT __stdcall CreatePixelShader(const DWORD *pFunction, IDirect3DPixelShader9 **ppShader) = 0;
    virtual HRESULT __stdcall SetPixelShader(IDirect3DPixelShader9 *pShader) = 0;
    virtual HRESULT __stdcall GetPixelShader(IDirect3DPixelShader9 **ppShader) = 0;
    virtual HRESULT __stdcall SetPixelShaderConstantF(UINT StartRegister, const float *pConstantData, UINT Vector4fCount) = 0;
    virtual HRESULT __stdcall GetPixelShaderConstantF(UINT StartRegister, float *pConstantData, UINT Vector4fCount) = 0;
    virtual HRESULT __stdcall SetPixelShaderConstantI(UINT StartRegister, const int *pConstantData, UINT Vector4iCount) = 0;
    virtual HRESULT __stdcall GetPixelShaderConstantI(UINT StartRegister, int *pConstantData, UINT Vector4iCount) = 0;
    virtual HRESULT __stdcall SetPixelShaderConstantB(UINT StartRegister, const BOOL *pConstantData, UINT BoolCount) = 0;
    virtual HRESULT __stdcall GetPixelShaderConstantB(UINT StartRegister, BOOL *pConstantData, UINT BoolCount) = 0;
    virtual HRESULT __stdcall DrawRectPatch(UINT Handle, const float *pNumSegs, const D3DRECTPATCH_INFO *pRectPatchInfo) = 0;
    virtual HRESULT __stdcall DrawTriPatch(UINT Handle, const float *pNumSegs, const D3DTRIPATCH_INFO *pTriPatchInfo) = 0;
    virtual HRESULT __stdcall DeletePatch(UINT Handle) = 0;
    virtual HRESULT __stdcall CreateQuery(D3DQUERYTYPE Type, IDirect3DQuery9 **ppQuery) = 0;
};

// Lo minimo de D3DX (d3dx9math.h del SDK de febrero de 2005) que usa el juego, enlazado contra d3dx9_26.dll.
// Los tipos llevan el mismo tamano y disposition que los del SDK; las funciones van sin dllimport porque el exe
// llama al thunk del import directo (`call D3DXMatrixMultiply`, no `call [__imp__...]`).
typedef struct _D3DXVECTOR3 {
    float x;
    float y;
    float z;
} D3DXVECTOR3;

typedef struct _D3DXVECTOR4 {
    float x;
    float y;
    float z;
    float w;
} D3DXVECTOR4;

// D3DXMATRIX hereda de D3DMATRIX (d3d9types.h) como en el SDK de C++: SetTransform la toma sin conversion.
struct D3DMATRIX {
    union {
        struct {
            float _11, _12, _13, _14;
            float _21, _22, _23, _24;
            float _31, _32, _33, _34;
            float _41, _42, _43, _44;
        };
        float m[4][4];
    };
};

typedef struct _D3DXMATRIX : public D3DMATRIX {
} D3DXMATRIX;

// La matriz alineada a 16 de d3dx9math.h (D3DX_ALIGN16): una local de este tipo obliga al marco con `and esp, -16`.
typedef struct _D3DXMATRIXA16 : public D3DXMATRIX {
    _D3DXMATRIXA16() {}
    _D3DXMATRIXA16(float f11, float f12, float f13, float f14, float f21, float f22, float f23, float f24, float f31,
                   float f32, float f33, float f34, float f41, float f42, float f43, float f44) {
        _11 = f11;
        _12 = f12;
        _13 = f13;
        _14 = f14;
        _21 = f21;
        _22 = f22;
        _23 = f23;
        _24 = f24;
        _31 = f31;
        _32 = f32;
        _33 = f33;
        _34 = f34;
        _41 = f41;
        _42 = f42;
        _43 = f43;
        _44 = f44;
    }
} __declspec(align(16)) D3DXMATRIXA16;

// d3dx9math.inl: la identidad, en linea. En speed.exe se guardan primero los doce ceros y luego la diagonal, cada uno
// en el orden de la matriz (la asignacion encadenada de los SDK posteriores los guardaria al reves).
inline D3DXMATRIX *D3DXMatrixIdentity(D3DXMATRIX *pOut) {
    pOut->m[0][1] = 0.0f;
    pOut->m[0][2] = 0.0f;
    pOut->m[0][3] = 0.0f;
    pOut->m[1][0] = 0.0f;
    pOut->m[1][2] = 0.0f;
    pOut->m[1][3] = 0.0f;
    pOut->m[2][0] = 0.0f;
    pOut->m[2][1] = 0.0f;
    pOut->m[2][3] = 0.0f;
    pOut->m[3][0] = 0.0f;
    pOut->m[3][1] = 0.0f;
    pOut->m[3][2] = 0.0f;
    pOut->m[0][0] = 1.0f;
    pOut->m[1][1] = 1.0f;
    pOut->m[2][2] = 1.0f;
    pOut->m[3][3] = 1.0f;
    return pOut;
}

// ID3DXEffect de d3dx9effect.h (febrero de 2005), en el orden del SDK hasta End: el juego llama por la vtable
// (SetTexture +0xD0, Begin +0xFC, BeginPass +0x100, CommitChanges +0x104, EndPass +0x108, End +0x10C).
typedef const char *D3DXHANDLE;
struct D3DXEFFECT_DESC {
    const char *Creator;
    UINT Parameters;
    UINT Techniques;
    UINT Functions;
};
struct D3DXPARAMETER_DESC {
    const char *Name;
    const char *Semantic;
    DWORD Class;
    DWORD Type;
    UINT Rows;
    UINT Columns;
    UINT Elements;
    UINT Annotations;
    UINT StructMembers;
    DWORD Flags;
    UINT Bytes;
};
struct D3DXTECHNIQUE_DESC {
    const char *Name;
    UINT Passes;
    UINT Annotations;
};
struct D3DXPASS_DESC;
struct D3DXFUNCTION_DESC;
struct ID3DXEffectPool;

struct ID3DXEffect {
    virtual HRESULT __stdcall QueryInterface(REFIID iid, void **ppv) = 0;
    virtual unsigned long __stdcall AddRef() = 0;
    virtual unsigned long __stdcall Release() = 0;
    virtual HRESULT __stdcall GetDesc(D3DXEFFECT_DESC *pDesc) = 0;
    virtual HRESULT __stdcall GetParameterDesc(D3DXHANDLE hParameter, D3DXPARAMETER_DESC *pDesc) = 0;
    virtual HRESULT __stdcall GetTechniqueDesc(D3DXHANDLE hTechnique, D3DXTECHNIQUE_DESC *pDesc) = 0;
    virtual HRESULT __stdcall GetPassDesc(D3DXHANDLE hPass, D3DXPASS_DESC *pDesc) = 0;
    virtual HRESULT __stdcall GetFunctionDesc(D3DXHANDLE hShader, D3DXFUNCTION_DESC *pDesc) = 0;
    virtual D3DXHANDLE __stdcall GetParameter(D3DXHANDLE hParameter, UINT Index) = 0;
    virtual D3DXHANDLE __stdcall GetParameterByName(D3DXHANDLE hParameter, const char *pName) = 0;
    virtual D3DXHANDLE __stdcall GetParameterBySemantic(D3DXHANDLE hParameter, const char *pSemantic) = 0;
    virtual D3DXHANDLE __stdcall GetParameterElement(D3DXHANDLE hParameter, UINT Index) = 0;
    virtual D3DXHANDLE __stdcall GetTechnique(UINT Index) = 0;
    virtual D3DXHANDLE __stdcall GetTechniqueByName(const char *pName) = 0;
    virtual D3DXHANDLE __stdcall GetPass(D3DXHANDLE hTechnique, UINT Index) = 0;
    virtual D3DXHANDLE __stdcall GetPassByName(D3DXHANDLE hTechnique, const char *pName) = 0;
    virtual D3DXHANDLE __stdcall GetFunction(UINT Index) = 0;
    virtual D3DXHANDLE __stdcall GetFunctionByName(const char *pName) = 0;
    virtual D3DXHANDLE __stdcall GetAnnotation(D3DXHANDLE hObject, UINT Index) = 0;
    virtual D3DXHANDLE __stdcall GetAnnotationByName(D3DXHANDLE hObject, const char *pName) = 0;
    virtual HRESULT __stdcall SetValue(D3DXHANDLE hParameter, const void *pData, UINT Bytes) = 0;
    virtual HRESULT __stdcall GetValue(D3DXHANDLE hParameter, void *pData, UINT Bytes) = 0;
    virtual HRESULT __stdcall SetBool(D3DXHANDLE hParameter, BOOL b) = 0;
    virtual HRESULT __stdcall GetBool(D3DXHANDLE hParameter, BOOL *pb) = 0;
    virtual HRESULT __stdcall SetBoolArray(D3DXHANDLE hParameter, const BOOL *pb, UINT Count) = 0;
    virtual HRESULT __stdcall GetBoolArray(D3DXHANDLE hParameter, BOOL *pb, UINT Count) = 0;
    virtual HRESULT __stdcall SetInt(D3DXHANDLE hParameter, int n) = 0;
    virtual HRESULT __stdcall GetInt(D3DXHANDLE hParameter, int *pn) = 0;
    virtual HRESULT __stdcall SetIntArray(D3DXHANDLE hParameter, const int *pn, UINT Count) = 0;
    virtual HRESULT __stdcall GetIntArray(D3DXHANDLE hParameter, int *pn, UINT Count) = 0;
    virtual HRESULT __stdcall SetFloat(D3DXHANDLE hParameter, float f) = 0;
    virtual HRESULT __stdcall GetFloat(D3DXHANDLE hParameter, float *pf) = 0;
    virtual HRESULT __stdcall SetFloatArray(D3DXHANDLE hParameter, const float *pf, UINT Count) = 0;
    virtual HRESULT __stdcall GetFloatArray(D3DXHANDLE hParameter, float *pf, UINT Count) = 0;
    virtual HRESULT __stdcall SetVector(D3DXHANDLE hParameter, const D3DXVECTOR4 *pVector) = 0;
    virtual HRESULT __stdcall GetVector(D3DXHANDLE hParameter, D3DXVECTOR4 *pVector) = 0;
    virtual HRESULT __stdcall SetVectorArray(D3DXHANDLE hParameter, const D3DXVECTOR4 *pVector, UINT Count) = 0;
    virtual HRESULT __stdcall GetVectorArray(D3DXHANDLE hParameter, D3DXVECTOR4 *pVector, UINT Count) = 0;
    virtual HRESULT __stdcall SetMatrix(D3DXHANDLE hParameter, const D3DXMATRIX *pMatrix) = 0;
    virtual HRESULT __stdcall GetMatrix(D3DXHANDLE hParameter, D3DXMATRIX *pMatrix) = 0;
    virtual HRESULT __stdcall SetMatrixArray(D3DXHANDLE hParameter, const D3DXMATRIX *pMatrix, UINT Count) = 0;
    virtual HRESULT __stdcall GetMatrixArray(D3DXHANDLE hParameter, D3DXMATRIX *pMatrix, UINT Count) = 0;
    virtual HRESULT __stdcall SetMatrixPointerArray(D3DXHANDLE hParameter, const D3DXMATRIX **ppMatrix, UINT Count) = 0;
    virtual HRESULT __stdcall GetMatrixPointerArray(D3DXHANDLE hParameter, D3DXMATRIX **ppMatrix, UINT Count) = 0;
    virtual HRESULT __stdcall SetMatrixTranspose(D3DXHANDLE hParameter, const D3DXMATRIX *pMatrix) = 0;
    virtual HRESULT __stdcall GetMatrixTranspose(D3DXHANDLE hParameter, D3DXMATRIX *pMatrix) = 0;
    virtual HRESULT __stdcall SetMatrixTransposeArray(D3DXHANDLE hParameter, const D3DXMATRIX *pMatrix, UINT Count) = 0;
    virtual HRESULT __stdcall GetMatrixTransposeArray(D3DXHANDLE hParameter, D3DXMATRIX *pMatrix, UINT Count) = 0;
    virtual HRESULT __stdcall SetMatrixTransposePointerArray(D3DXHANDLE hParameter, const D3DXMATRIX **ppMatrix,
                                                             UINT Count) = 0;
    virtual HRESULT __stdcall GetMatrixTransposePointerArray(D3DXHANDLE hParameter, D3DXMATRIX **ppMatrix,
                                                             UINT Count) = 0;
    virtual HRESULT __stdcall SetString(D3DXHANDLE hParameter, const char *pString) = 0;
    virtual HRESULT __stdcall GetString(D3DXHANDLE hParameter, const char **ppString) = 0;
    virtual HRESULT __stdcall SetTexture(D3DXHANDLE hParameter, IDirect3DBaseTexture9 *pTexture) = 0;
    virtual HRESULT __stdcall GetTexture(D3DXHANDLE hParameter, IDirect3DBaseTexture9 **ppTexture) = 0;
    virtual HRESULT __stdcall GetPixelShader(D3DXHANDLE hParameter, IDirect3DPixelShader9 **ppPShader) = 0;
    virtual HRESULT __stdcall GetVertexShader(D3DXHANDLE hParameter, IDirect3DVertexShader9 **ppVShader) = 0;
    virtual HRESULT __stdcall SetArrayRange(D3DXHANDLE hParameter, UINT uStart, UINT uEnd) = 0;
    virtual HRESULT __stdcall GetPool(ID3DXEffectPool **ppPool) = 0;
    virtual HRESULT __stdcall SetTechnique(D3DXHANDLE hTechnique) = 0;
    virtual D3DXHANDLE __stdcall GetCurrentTechnique() = 0;
    virtual HRESULT __stdcall ValidateTechnique(D3DXHANDLE hTechnique) = 0;
    virtual HRESULT __stdcall FindNextValidTechnique(D3DXHANDLE hTechnique, D3DXHANDLE *pTechnique) = 0;
    virtual BOOL __stdcall IsParameterUsed(D3DXHANDLE hParameter, D3DXHANDLE hTechnique) = 0;
    virtual HRESULT __stdcall Begin(UINT *pPasses, DWORD Flags) = 0;
    virtual HRESULT __stdcall BeginPass(UINT Pass) = 0;
    virtual HRESULT __stdcall CommitChanges() = 0;
    virtual HRESULT __stdcall EndPass() = 0;
    virtual HRESULT __stdcall End() = 0;
    virtual HRESULT __stdcall GetDevice(IDirect3DDevice9 **ppDevice) = 0;
    virtual HRESULT __stdcall OnLostDevice() = 0;
    virtual HRESULT __stdcall OnResetDevice() = 0;
};

struct ID3DXEffectPool {
    virtual HRESULT __stdcall QueryInterface(REFIID iid, void **ppv) = 0;
    virtual unsigned long __stdcall AddRef() = 0;
    virtual unsigned long __stdcall Release() = 0;
};

#define WINAPI __stdcall

extern "C" HRESULT WINAPI D3DXCreateEffectPool(ID3DXEffectPool **ppPool);
struct ID3DXBuffer;
struct ID3DXInclude;
struct D3DXMACRO;
typedef struct HINSTANCE__ *HMODULE;
extern "C" HRESULT WINAPI D3DXCreateEffectFromResourceA(IDirect3DDevice9 *pDevice, HMODULE hSrcModule,
                                                       const char *pSrcResource, const D3DXMACRO *pDefines,
                                                       ID3DXInclude *pInclude, DWORD Flags, ID3DXEffectPool *pPool,
                                                       ID3DXEffect **ppEffect, ID3DXBuffer **ppCompilationErrors);
extern "C" D3DXMATRIX *WINAPI D3DXMatrixMultiply(D3DXMATRIX *pOut, const D3DXMATRIX *pM1, const D3DXMATRIX *pM2);
extern "C" D3DXVECTOR4 *WINAPI D3DXVec4Transform(D3DXVECTOR4 *pOut, const D3DXVECTOR4 *pV, const D3DXMATRIX *pM);
extern "C" D3DXVECTOR4 *WINAPI D3DXVec3Transform(D3DXVECTOR4 *pOut, const D3DXVECTOR3 *pV, const D3DXMATRIX *pM);

#endif
