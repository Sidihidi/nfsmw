/* recon.inl -- el .mdebug de la A124 y el DWARF de GameCube dicen que el criticalpath.c de cada plataforma lo
 * incluye (D:/env/egami/vp6/dev/source/decode/ps2/../cmn/recon.inl en PS2, 1.0.3; gc/../cmn en GameCube, 1.0.6). */

static int BicubicFilters[8][4] = {
    {0, 128, 0, 0},   {-4, 118, 16, -2}, {-7, 106, 34, -5}, {-8, 90, 53, -7},
    {-8, 72, 72, -8}, {-7, 53, 90, -8},  {-5, 34, 106, -7}, {-2, 16, 118, -4},
};

static int BilinearFilters[8][2] = {
    {128, 0}, {112, 16}, {96, 32}, {80, 48}, {64, 64}, {48, 80}, {32, 96}, {16, 112},
};

void VP6_PredictFilteredBlock(struct PB_INSTANCE *pbi, short *OutputPtr, BLOCK_POSITION bp) {
    unsigned char *SrcPtr;
    unsigned char *TempBuffer;
    unsigned int TempPtr1;
    unsigned int TempPtr2;
    int ModX;
    int ModY;
    unsigned int IVar;
    unsigned int BicMvSizeLimit;
    unsigned int Stride;
    unsigned int MvShift;
    unsigned int MvModMask;

    MvShift = pbi->mbi.MvShift;
    MvModMask = pbi->mbi.MvModMask;

    SrcPtr = pbi->LastFrameRecon;

    if (VP6_Mode2Frame[pbi->mbi.Mode] == 2) {
        SrcPtr = pbi->GoldenFrame;
    }

    if (pbi->VpProfile == 0 || pbi->UseLoopFilter == 0) {
        int mVx = pbi->mbi.Mv[bp].x;
        int mVy = pbi->mbi.Mv[bp].y;
        int mx;
        int my;

        ModX = mVx & MvModMask;
        ModY = mVy & MvModMask;

        mVx += MvModMask & (mVx >> 31);
        mVy += MvModMask & (mVy >> 31);

        mx = mVx >> MvShift;
        my = mVy >> MvShift;

        TempBuffer = SrcPtr + pbi->mbi.Recon + (pbi->mbi.FrameReconStride * my + mx);
        Stride = pbi->mbi.CurrentReconStride;
        TempPtr1 = TempPtr2 = 0;
    } else {
        VP6_PredictFiltered(pbi, SrcPtr + pbi->mbi.Recon, pbi->mbi.Mv[bp].x, pbi->mbi.Mv[bp].y);
        TempBuffer = pbi->LoopFilteredBlock;
        Stride = 16;
        TempPtr1 = 34;
        TempPtr2 = 34;

        ModX = pbi->mbi.Mv[bp].x & pbi->mbi.MvModMask;
        ModY = pbi->mbi.Mv[bp].y & pbi->mbi.MvModMask;
    }

    if (ModX) {
        if (pbi->mbi.Mv[bp].x > 0) {
            TempPtr2++;
        } else {
            TempPtr2--;
        }
    }

    if (ModY) {
        if (pbi->mbi.Mv[bp].y <= 0) {
            TempPtr2 -= Stride;
        } else {
            TempPtr2 += Stride;
        }
    }

    if (TempPtr1 != TempPtr2) {
        if (bp <= 3) {
            ModX = ModX << 1;
            ModY = ModY << 1;

            if (pbi->VpProfile == 0) {
                FilterBlock(TempBuffer + TempPtr1, TempBuffer + TempPtr2,
                            (unsigned short *)OutputPtr, Stride, ModX, ModY, 0);
                return;
            }

            if (pbi->PredictionFilterMode == 2) {
                if (pbi->PredictionFilterMvSizeThresh) {
                    BicMvSizeLimit = 4 << (pbi->PredictionFilterMvSizeThresh - 1);
                } else {
                    BicMvSizeLimit = 128;
                }

                if (pbi->PredictionFilterMvSizeThresh &&
                    (abs(pbi->mbi.Mv[bp].x) > BicMvSizeLimit ||
                     abs(pbi->mbi.Mv[bp].y) > BicMvSizeLimit)) {
                    FilterBlock(TempBuffer + TempPtr1, TempBuffer + TempPtr2,
                                (unsigned short *)OutputPtr, Stride, ModX, ModY, 0);
                    return;
                }

                if (pbi->PredictionFilterVarThresh) {
                    IVar = Var16Point(TempBuffer + TempPtr1, Stride);
                    FilterBlock(TempBuffer + TempPtr1, TempBuffer + TempPtr2,
                                (unsigned short *)OutputPtr, Stride, ModX, ModY,
                                IVar >= pbi->PredictionFilterVarThresh);
                } else {
                    FilterBlock(TempBuffer + TempPtr1, TempBuffer + TempPtr2,
                                (unsigned short *)OutputPtr, Stride, ModX, ModY, 1);
                }
                return;
            } else {
                FilterBlock(TempBuffer + TempPtr1, TempBuffer + TempPtr2,
                            (unsigned short *)OutputPtr, Stride, ModX, ModY,
                            pbi->PredictionFilterMode == 1);
                return;
            }
        }

        FilterBlock(TempBuffer + TempPtr1, TempBuffer + TempPtr2, (unsigned short *)OutputPtr,
                    Stride, ModX, ModY, 0);
    } else {
        UnpackBlock(TempBuffer + TempPtr1, OutputPtr, Stride);
    }
}

void VP6_ReconstructBlock(struct PB_INSTANCE *pbi, BLOCK_POSITION bp) {
    if (pbi->mbi.Mode == CODE_INTER_NO_MV) {
        ReconInter(pbi->TmpDataBuffer, pbi->ThisFrameRecon + pbi->mbi.Recon,
                   pbi->LastFrameRecon + pbi->mbi.Recon, pbi->ReconDataBuffer,
                   pbi->mbi.CurrentReconStride);
    } else if (VP6_ModeUsesMC[pbi->mbi.Mode]) {
        VP6_PredictFilteredBlock(pbi, pbi->TmpDataBuffer, bp);
        ReconBlock(pbi->TmpDataBuffer, pbi->ReconDataBuffer,
                   pbi->ThisFrameRecon + pbi->mbi.Recon, pbi->mbi.CurrentReconStride);
    } else if (pbi->mbi.Mode == CODE_USING_GOLDEN) {
        ReconInter(pbi->TmpDataBuffer, pbi->ThisFrameRecon + pbi->mbi.Recon,
                   pbi->GoldenFrame + pbi->mbi.Recon, pbi->ReconDataBuffer,
                   pbi->mbi.CurrentReconStride);
    } else {
        ReconIntra(pbi->TmpDataBuffer, pbi->ThisFrameRecon + pbi->mbi.Recon,
                   pbi->ReconDataBuffer, pbi->mbi.CurrentReconStride);
    }
}
