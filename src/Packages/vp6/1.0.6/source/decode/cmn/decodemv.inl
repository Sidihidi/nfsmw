/* decodemv.inl -- el .mdebug de la A124 y el DWARF de GameCube dicen que el criticalpath.c de cada plataforma lo
 * incluye (D:/env/egami/vp6/dev/source/decode/ps2/../cmn/decodemv.inl en PS2, 1.0.3; gc/../cmn en GameCube, 1.0.6). */

void VP6_decodeMotionVector(struct PB_INSTANCE *pbi, MOTION_VECTOR *mv, CODING_MODE Mode) {
    unsigned int i;
    int Vector;
    int SignBit;
    int MvOffsetX = 0;
    int MvOffsetY = 0;

    if (Mode == CODE_INTER_PLUS_MV) {
        if (pbi->mbi.NearestMvIndex <= 1) {
            MvOffsetX = pbi->mbi.NearestInterMVect.x;
            MvOffsetY = pbi->mbi.NearestInterMVect.y;
        }
    } else {
        if (pbi->mbi.NearestGMvIndex <= 1) {
            MvOffsetX = pbi->mbi.NearestGoldMVect.x;
            MvOffsetY = pbi->mbi.NearestGoldMVect.y;
        }
    }

    for (i = 0; i < 2; i++) {
        if (VP6_DecodeBool(&pbi->br, pbi->IsMvShortProb[i]) == 0) {
            if (VP6_DecodeBool(&pbi->br, pbi->MvShortProbs[i][0])) {
                if (VP6_DecodeBool(&pbi->br, pbi->MvShortProbs[i][4])) {
                    Vector = 6 + VP6_DecodeBool(&pbi->br, pbi->MvShortProbs[i][6]);
                } else {
                    Vector = 4 + VP6_DecodeBool(&pbi->br, pbi->MvShortProbs[i][5]);
                }
            } else {
                if (VP6_DecodeBool(&pbi->br, pbi->MvShortProbs[i][1])) {
                    Vector = 2 + VP6_DecodeBool(&pbi->br, pbi->MvShortProbs[i][3]);
                } else {
                    Vector = VP6_DecodeBool(&pbi->br, pbi->MvShortProbs[i][2]);
                }
            }
        } else {
            Vector = VP6_DecodeBool(&pbi->br, pbi->MvSizeProbs[i][0]);
            Vector += VP6_DecodeBool(&pbi->br, pbi->MvSizeProbs[i][1]) << 1;
            Vector += VP6_DecodeBool(&pbi->br, pbi->MvSizeProbs[i][2]) << 2;
            Vector += VP6_DecodeBool(&pbi->br, pbi->MvSizeProbs[i][7]) << 7;
            Vector += VP6_DecodeBool(&pbi->br, pbi->MvSizeProbs[i][6]) << 6;
            Vector += VP6_DecodeBool(&pbi->br, pbi->MvSizeProbs[i][5]) << 5;
            Vector += VP6_DecodeBool(&pbi->br, pbi->MvSizeProbs[i][4]) << 4;

            if (Vector & 0xF0) {
                Vector += VP6_DecodeBool(&pbi->br, pbi->MvSizeProbs[i][3]) << 3;
            } else {
                Vector += 8;
            }
        }

        if (Vector != 0) {
            SignBit = VP6_DecodeBool(&pbi->br, pbi->MvSignProbs[i]);
            if (SignBit) {
                Vector = -Vector;
            }
        }

        if (i != 0) {
            mv->y = Vector + MvOffsetY;
        } else {
            mv->x = Vector + MvOffsetX;
        }
    }
}

void VP6_FindNearestandNextNearest(struct PB_INSTANCE *pbi, unsigned int MBrow, unsigned int MBcol,
                                   unsigned char Frame, int *type) {
    int i;
    unsigned int OffsetMB;
    unsigned int BaseMB;
    int Nearest = 0;
    int NextNearest = 0;
    int nearestIndex;
    unsigned int thisMv;
    int typet;

    typet = 1;

    BaseMB = MBrow * pbi->MBCols + MBcol;

    for (i = 0; i < 12; i++) {
        OffsetMB = pbi->mvNearOffset[i] + BaseMB;

        if (VP6_Mode2Frame[pbi->predictionMode[OffsetMB]] != Frame) {
            continue;
        }

        thisMv = *((unsigned int *)&pbi->MBMotionVector[OffsetMB]);

        if (thisMv) {
            *((unsigned int *)&Nearest) = thisMv;
            typet = 2;
            break;
        }
    }

    nearestIndex = i;

    for (i = i + 1; i < 12; i++) {
        OffsetMB = pbi->mvNearOffset[i] + BaseMB;

        if (VP6_Mode2Frame[pbi->predictionMode[OffsetMB]] != Frame) {
            continue;
        }

        thisMv = *((unsigned int *)&pbi->MBMotionVector[OffsetMB]);

        if (thisMv == *((unsigned int *)&Nearest)) {
            continue;
        }

        if (thisMv) {
            *((unsigned int *)&NextNearest) = thisMv;
            typet = 0;
            break;
        }
    }

    if (Frame == 1) {
        *type = typet;
        pbi->mbi.NearestMvIndex = nearestIndex;
        *(unsigned int *)&pbi->mbi.NearestInterMVect = *(unsigned int *)&Nearest;
        *(unsigned int *)&pbi->mbi.NearInterMVect = *(unsigned int *)&NextNearest;
    } else {
        pbi->mbi.NearestGMvIndex = nearestIndex;
        *(unsigned int *)&pbi->mbi.NearestGoldMVect = *(unsigned int *)&Nearest;
        *(unsigned int *)&pbi->mbi.NearGoldMVect = *(unsigned int *)&NextNearest;
    }
}
