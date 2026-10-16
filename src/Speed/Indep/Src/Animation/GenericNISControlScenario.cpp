#include "GenericNISControlScenario.hpp"

// A124/SLUS y X360: 1e-6 exacto; GC: (double)1e-6f; speed.exe: 0.001 (los tres fcomp de HandleEventMessage)
#ifdef EA_PLATFORM_WIN32
#define NIS_EPSILON 0.001
#else
#define NIS_EPSILON 1e-6
#endif

void GenericNISControlScenario::HandleEventMessage(CWorldAnimEntityTree *entity_tree, uint32 wa_msg, void *data) {
    if (wa_msg == WAM_NIS_GENERIC_CONTROL_MSG) {
        GenericNISControlScenarioInfo *gcsi = static_cast<GenericNISControlScenarioInfo *>(data);
        bool set_to_pos_immediately = fabs(gcsi->len) < NIS_EPSILON;
        int open = fabs(gcsi->pos - 1.0f) < NIS_EPSILON ? 1 : (fabs(gcsi->pos) < NIS_EPSILON ? 0 : -1);
        float frame_len = gcsi->len;

        for (bPNode *node = entity_tree->instantiated_world_anim_entities.GetHead();
             node != entity_tree->instantiated_world_anim_entities.EndOfList(); node = node->GetNext()) {
            CWorldAnimEntity *entity = reinterpret_cast<CWorldAnimEntity *>(node->GetObject());
            if (gcsi->entity_hash == entity->GetInstanceNameHash()) {
                CWorldAnimCtrl *anim_ctrl = entity->GetAnimCtrl();
                if (anim_ctrl) {
                    if (set_to_pos_immediately) {
                        if (open == 1) {
                            anim_ctrl->JumpToEndForTrigger();
                        } else if (open == 0) {
                            anim_ctrl->JumpToBeginForTrigger();
                        } else {
                            anim_ctrl->SetEvalTime(gcsi->pos * anim_ctrl->GetAnimLength());
                        }
                    } else {
                        float scaling_factor;
                        anim_ctrl->SetTimeScale(1.0f);
                        anim_ctrl->ClearFlags(0x180);
                        scaling_factor = anim_ctrl->GetAnimLength() / frame_len;
                        anim_ctrl->ApplySpeedModifier(scaling_factor);
                        anim_ctrl->Play();
                    }
                }
            }
        }
    }
}
