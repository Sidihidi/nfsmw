#include "FnEventBlender.h"
#include "Speed/Indep/Src/EAGL4Anim/AnimTypeId.h"
#include "Speed/Indep/Src/EAGL4Anim/AnimUtil.h"

namespace EAGL4Anim {

void FnEventBlender::Eval(float previousTime, float currentTime, float *data) {
    if (currentTime <= mStartTransTime) {

        mAnim[0]->Eval(previousTime - mTimeOffset[0], currentTime - mTimeOffset[0], data);

    } else if (currentTime >= mEndTransTime) {

        mAnim[1]->Eval(previousTime - mTimeOffset[1], currentTime - mTimeOffset[1], data);

    } else {

        switch (mTriggerType) {

        case FIRST_ONLY:

            mAnim[0]->Eval(previousTime - mTimeOffset[0], currentTime - mTimeOffset[0], data);
            break;

        case SECOND_ONLY:

            mAnim[1]->Eval(previousTime - mTimeOffset[1], currentTime - mTimeOffset[1], data);
            break;

        default:

            mAnim[0]->Eval(previousTime - mTimeOffset[0], currentTime - mTimeOffset[0], data);

            mAnim[1]->Eval(previousTime - mTimeOffset[1], currentTime - mTimeOffset[1], data);
            break;
        }
    }
}

}; // namespace EAGL4Anim
