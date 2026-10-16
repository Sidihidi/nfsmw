#include "Speed/Indep/Src/Camera/Camera.hpp"
#include "Speed/Indep/Src/Ecstasy/Ecstasy.hpp"
#include "Speed/Indep/Src/Ecstasy/EcstasyData.hpp"
#include "Speed/Indep/Src/Ecstasy/eMath.hpp"
#include "Speed/Indep/bWare/Inc/bMath.hpp"
#include "Speed/PSX2/Src/Ecstasy/EcstasyE.hpp"
#include "Speed/PSX2/Src/Ecstasy/eViewPlat.hpp"

eViewPlatInfo ViewPlatInfoTable[26];

// Transforma las ocho esquinas de la caja y acumula las banderas de recorte del VU0 en dos pasadas.
eVisibleState ClassifyAABB(const bVector3 *aabb_min, const bVector3 *aabb_max, bMatrix4 *world_clip) {
    int near_flags;
    int far_flags;

    asm __volatile__("lqc2 vf2, 0(%2)\n"
                     "lqc2 vf3, 0(%3)\n"
                     "lqc2 vf4, 0(%4)\n"
                     "lqc2 vf5, 16(%4)\n"
                     "lqc2 vf6, 32(%4)\n"
                     "lqc2 vf7, 48(%4)\n"
                     "ctc2 %0, $vi18\n"
                     "nop\n"
                     "vsub.xyzw vf3, vf3, vf2\n"
                     "vmulax.xyzw ACC, vf4, vf2x\n"
                     "vmadday.xyzw ACC, vf5, vf2y\n"
                     "vmaddaz.xyzw ACC, vf6, vf2z\n"
                     "vmaddw.xyzw vf8, vf7, vf0w\n"
                     "vmulx.xyzw vf9, vf4, vf3x\n"
                     "vmuly.xyzw vf10, vf5, vf3y\n"
                     "vmulz.xyzw vf11, vf6, vf3z\n"
                     "vadd.xyzw vf17, vf8, vf9\n"
                     "vadd.xyzw vf18, vf8, vf9\n"
                     "vadd.xyzw vf18, vf18, vf10\n"
                     "vadd.xyzw vf19, vf8, vf10\n"
                     "vclipw.xyz vf8, vf8w\n"
                     "vclipw.xyz vf17, vf17w\n"
                     "vclipw.xyz vf18, vf18w\n"
                     "vclipw.xyz vf19, vf19w\n"
                     "vadd.xyzw vf8, vf8, vf11\n"
                     "vadd.xyzw vf17, vf17, vf11\n"
                     "vadd.xyzw vf18, vf18, vf11\n"
                     "vadd.xyzw vf19, vf19, vf11\n"
                     "vnop\n"
                     "cfc2 %0, $vi18\n"
                     "nop\n"
                     "vclipw.xyz vf8, vf8w\n"
                     "vclipw.xyz vf17, vf17w\n"
                     "vclipw.xyz vf18, vf18w\n"
                     "vclipw.xyz vf19, vf19w\n"
                     "vnop\n"
                     "vnop\n"
                     "vnop\n"
                     "vnop\n"
                     "vnop\n"
                     "cfc2 %1, $vi18\n"
                     "nop"
                     : "=&r"(near_flags), "=r"(far_flags)
                     : "r"(aabb_min), "r"(aabb_max), "r"(world_clip));
    if ((near_flags | far_flags) == 0) {
        return EVISIBLESTATE_FULL;
    }

    // unsigned (rondas 19: probados `int` —saca sra en los dos desplazamientos, 4 filas— e `int` con
    // `(unsigned)outside >> N` —srl correctos pero sin la extension de signo, 2 filas—; el original extiende
    // con signo (dsll32/dsra32) y desplaza en logico (srl), combinacion sin forma conocida).
    unsigned int outside = near_flags & far_flags;

    outside &= outside >> 12;
    outside &= outside >> 6;
    return (outside & 0x3f) == 0 ? EVISIBLESTATE_PARTIAL : EVISIBLESTATE_NOT;
}

eVisibleState ClassifyPoint(const bVector3 *point, bMatrix4 *world_clip) {
    int flags;

    asm __volatile__("lqc2 vf2, 0(%1)\n"
                     "lqc2 vf4, 0(%2)\n"
                     "lqc2 vf5, 16(%2)\n"
                     "lqc2 vf6, 32(%2)\n"
                     "lqc2 vf7, 48(%2)\n"
                     "ctc2 $0, $vi18\n"
                     "vnop\n"
                     "vmulax.xyzw ACC, vf4, vf2x\n"
                     "vmadday.xyzw ACC, vf5, vf2y\n"
                     "vmaddaz.xyzw ACC, vf6, vf2z\n"
                     "vmaddw.xyzw vf8, vf7, vf0w\n"
                     "vnop\n"
                     "vnop\n"
                     "vnop\n"
                     "vnop\n"
                     "vnop\n"
                     "vclipw.xyz vf8, vf8w\n"
                     "vnop\n"
                     "vnop\n"
                     "vnop\n"
                     "vnop\n"
                     "vnop\n"
                     "cfc2 %0, $vi18\n"
                     "nop"
                     : "=r"(flags)
                     : "r"(point), "r"(world_clip));
    return flags == 0 ? EVISIBLESTATE_FULL : EVISIBLESTATE_NOT;
}

eVisibleState eViewPlatInterface::GetVisibleStateSB(const bVector3 *aabb_min, const bVector3 *aabb_max, bMatrix4 *local_world) {
    eViewPlatInfo *plat_info = GetPlatInfo();
    bMatrix4 *world_clip = plat_info->GetWorldClipGBMatrix();
    bMatrix4 local_clip;

    if (local_world) {
        eMulMatrix(&local_clip, local_world, world_clip);
        world_clip = &local_clip;
    }
    return ClassifyAABB(aabb_min, aabb_max, world_clip);
}

eVisibleState eViewPlatInterface::GetVisibleStateSB(const bVector3 *position, bMatrix4 *local_world) {
    eViewPlatInfo *plat_info = GetPlatInfo();
    bMatrix4 *world_clip = plat_info->GetWorldClipGBMatrix();
    bMatrix4 local_clip;

    if (local_world) {
        eMulMatrix(&local_clip, local_world, world_clip);
        world_clip = &local_clip;
    }
    return ClassifyPoint(position, world_clip);
}

void eViewPlatInterface::GetScreenPosition(bVector3 *screen_position, const bVector3 *world_position) {
    eView *view = static_cast<eView *>(this);
    eViewPlatInfo *plat_info = GetPlatInfo();
    eRenderTarget *render_target = view->GetRenderTarget();
    bVector4 world;
    bVector4 screen;

    world.x = world_position->x;
    world.y = world_position->y;
    world.z = world_position->z;
    world.w = 1.0f;
    eRotTransPers(&screen, plat_info->GetWorldScreenMatrix(), &world);

    int half_height = render_target->ScissorH / 2;
    int half_width = render_target->ScissorW / 2;

    screen_position->z = screen.z;
    screen_position->x = (screen.x - 2048.0f) + half_width;
    screen_position->y = (screen.y - 2048.0f) + half_height;
}

int eViewPlatInterface::GetPixelWidth() {
    eView *view = static_cast<eView *>(this);
    eRenderTarget *render_target = view->GetRenderTarget();

    return render_target->ScissorW;
}

// Las matrices de la vista para el VU: se piden al frame malloc (ocho seguidas) y, con la vista activa, se calculan
// desde la camara. Cada una de las tres de recorte con banda de guarda del VU lleva la escala y la traslacion de su
// matriz de pantalla y de la inversa.
void eViewPlatInfo::CalculateViewMatricies(eView *view, float force_near_z, float force_far_z, float force_screen_far_z) {
    Camera *camera = view->GetCamera();
    bMatrix4 view_clipgb;
    bMatrix4 clipgb_screen0;
    bMatrix4 clipgb_screen1;
    bMatrix4 clipgb_screen2;
    bMatrix4 *matricies = static_cast<bMatrix4 *>(eFrameMalloc(sizeof(bMatrix4) * 8));
    bMatrix4 *world_view = &matricies[0];
    bMatrix4 *world_clipgb = &matricies[1];
    bMatrix4 *world_screen = &matricies[2];
    bMatrix4 *clipgb_screen = &matricies[3];
    bMatrix4 *view_screen = &matricies[4];
    bMatrix4 *vuclipgb_screen0 = &matricies[5];
    bMatrix4 *vuclipgb_screen1 = &matricies[6];
    bMatrix4 *vuclipgb_screen2 = &matricies[7];

    if (!matricies) {
        world_view = eGetIdentityMatrix();
        world_clipgb = eGetIdentityMatrix();
        world_screen = eGetIdentityMatrix();
        clipgb_screen = eGetIdentityMatrix();
        view_screen = eGetIdentityMatrix();
        vuclipgb_screen0 = eGetIdentityMatrix();
        vuclipgb_screen1 = eGetIdentityMatrix();
        vuclipgb_screen2 = eGetIdentityMatrix();
    } else if (view->IsActive()) {
        CreateViewMatricies(view, view_screen, &view_clipgb, &clipgb_screen0, &clipgb_screen1, &clipgb_screen2, force_near_z,
                            force_far_z, force_screen_far_z);

        bMatrix4 sc0;
        bMatrix4 sc1;
        bMatrix4 sc2;
        bMatrix4 clipsb_screen0;

        fInvertMatrix(&sc0, &clipgb_screen0);
        fInvertMatrix(&sc1, &clipgb_screen1);
        fInvertMatrix(&sc2, &clipgb_screen2);

        vuclipgb_screen0->v0.x = clipgb_screen0.v0.x;
        vuclipgb_screen0->v0.y = clipgb_screen0.v1.y;
        vuclipgb_screen0->v0.z = clipgb_screen0.v2.z;
        vuclipgb_screen0->v0.w = 0.0f;
        vuclipgb_screen0->v1.x = clipgb_screen0.v3.x;
        vuclipgb_screen0->v1.y = clipgb_screen0.v3.y;
        vuclipgb_screen0->v1.z = clipgb_screen0.v3.z;
        vuclipgb_screen0->v1.w = clipgb_screen0.v3.w;

        vuclipgb_screen0->v2.x = sc0.v0.x;
        vuclipgb_screen0->v2.y = sc0.v1.y;
        vuclipgb_screen0->v2.z = sc0.v2.z;
        vuclipgb_screen0->v2.w = 0.0f;
        vuclipgb_screen0->v3.x = sc0.v3.x;
        vuclipgb_screen0->v3.y = sc0.v3.y;
        vuclipgb_screen0->v3.z = sc0.v3.z;
        vuclipgb_screen0->v3.w = sc0.v3.w;

        vuclipgb_screen1->v0.x = clipgb_screen1.v0.x;
        vuclipgb_screen1->v0.y = clipgb_screen1.v1.y;
        vuclipgb_screen1->v0.z = clipgb_screen1.v2.z;
        vuclipgb_screen1->v0.w = 0.0f;
        vuclipgb_screen1->v1.x = clipgb_screen1.v3.x;
        vuclipgb_screen1->v1.y = clipgb_screen1.v3.y;
        vuclipgb_screen1->v1.z = clipgb_screen1.v3.z;
        vuclipgb_screen1->v1.w = clipgb_screen1.v3.w;

        vuclipgb_screen1->v2.x = sc1.v0.x;
        vuclipgb_screen1->v2.y = sc1.v1.y;
        vuclipgb_screen1->v2.z = sc1.v2.z;
        vuclipgb_screen1->v2.w = 0.0f;
        vuclipgb_screen1->v3.x = sc1.v3.x;
        vuclipgb_screen1->v3.y = sc1.v3.y;
        vuclipgb_screen1->v3.z = sc1.v3.z;
        vuclipgb_screen1->v3.w = sc1.v3.w;

        vuclipgb_screen2->v0.x = clipgb_screen2.v0.x;
        vuclipgb_screen2->v0.y = clipgb_screen2.v1.y;
        vuclipgb_screen2->v0.z = clipgb_screen2.v2.z;
        vuclipgb_screen2->v0.w = 0.0f;
        vuclipgb_screen2->v1.x = clipgb_screen2.v3.x;
        vuclipgb_screen2->v1.y = clipgb_screen2.v3.y;
        vuclipgb_screen2->v1.z = clipgb_screen2.v3.z;
        vuclipgb_screen2->v1.w = clipgb_screen2.v3.w;

        vuclipgb_screen2->v2.x = sc2.v0.x;
        vuclipgb_screen2->v2.y = sc2.v1.y;
        vuclipgb_screen2->v2.z = sc2.v2.z;
        vuclipgb_screen2->v2.w = 0.0f;
        vuclipgb_screen2->v3.x = sc2.v3.x;
        vuclipgb_screen2->v3.y = sc2.v3.y;
        vuclipgb_screen2->v3.z = sc2.v3.z;
        vuclipgb_screen2->v3.w = sc2.v3.w;

        *world_view = *camera->GetCameraMatrix();
        *clipgb_screen = clipgb_screen0;

        eMulMatrix(world_clipgb, world_view, &view_clipgb);
        eMulMatrix(world_screen, world_view, view_screen);
    }

    if (view->IsActive()) {
        SetWorldViewMatrix(world_view);
        SetWorldScreenMatrix(world_screen);
        SetWorldClipGBMatrix(world_clipgb);
        SetViewScreenMatrix(view_screen);
        SetVUClipGBScreenMatrix0(vuclipgb_screen0);
        SetVUClipGBScreenMatrix1(vuclipgb_screen1);
        SetVUClipGBScreenMatrix2(vuclipgb_screen2);

        eRenderTarget *render_target = view->GetRenderTarget();

        if (render_target && render_target->IsActive()) {
            render_target->WorldScreen = world_screen;
            render_target->WorldView = world_view;
            render_target->WorldClipGB = world_clipgb;
            render_target->ClipGBScreen = clipgb_screen;
            render_target->VUClipGBScreen0 = vuclipgb_screen0;
            render_target->VUClipGBScreen1 = vuclipgb_screen1;
            render_target->VUClipGBScreen2 = vuclipgb_screen2;
            render_target->SetupMatrixPacks();
        }
    }
}

eViewPlatInfo *eViewPlatInterface::GimmeMyViewPlatInfo(int view_id) {
    return &ViewPlatInfoTable[view_id];
}

#include "Speed/Indep/Src/Ecstasy/eSprites.hpp"

eSpriteManager SpriteManager;
