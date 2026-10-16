/* decodemode.inl -- el .mdebug de la A124 y el DWARF de GameCube dicen que el criticalpath.c de cada plataforma lo
 * incluye (D:/env/egami/vp6/dev/source/decode/ps2/../cmn/decodemode.inl en PS2, 1.0.3; gc/../cmn en GameCube, 1.0.6). */

CODING_MODE VP6_DecodeBlockMode(struct PB_INSTANCE *pbi) {
    int choice;

    choice = VP6_DecodeBool128(&pbi->br) * 2;
    choice = choice + VP6_DecodeBool128(&pbi->br);

#if defined(EA_PLATFORM_WIN32)
    /* PC (speed.exe 0x807239): los casos van de mayor a menor; MSVC emite los
     * cuerpos en ese orden y funde el epilogo con el primero. */
    switch (choice) {
    case 3:
        return CODE_INTER_NEAR_MV;
    case 2:
        return CODE_INTER_NEAREST_MV;
    case 1:
        return CODE_INTER_PLUS_MV;
    case 0:
        return CODE_INTER_NO_MV;
    default:
        return CODE_INTER_NO_MV;
    }
#else
    switch (choice) {
    case 1:
        return CODE_INTER_PLUS_MV;
    case 2:
        return CODE_INTER_NEAREST_MV;
    case 3:
        return CODE_INTER_NEAR_MV;
    case 0:
        return CODE_INTER_NO_MV;
    default:
        return CODE_INTER_NO_MV;
    }
#endif
}

CODING_MODE VP6_DecodeMode(struct PB_INSTANCE *pbi, CODING_MODE lastmode, unsigned int type) {
    CODING_MODE mode;

    if (VP6_DecodeBool(&pbi->br, pbi->probModeSame[type][lastmode])) {
        mode = lastmode;
    } else {
        unsigned char *Stats = pbi->probMode[type][lastmode];

        if (VP6_DecodeBool(&pbi->br, Stats[0])) {
            if (VP6_DecodeBool(&pbi->br, Stats[2])) {
                if (VP6_DecodeBool(&pbi->br, Stats[6])) {
                    mode = (CODING_MODE)(8 + VP6_DecodeBool(&pbi->br, Stats[8]));
                } else {
                    mode = (CODING_MODE)(5 + VP6_DecodeBool(&pbi->br, Stats[7]));
                }
            } else {
                mode = CODE_INTRA;
                if (VP6_DecodeBool(&pbi->br, Stats[5])) {
                    mode = CODE_INTER_FOURMV;
                }
            }
        } else {
            if (VP6_DecodeBool(&pbi->br, Stats[1])) {
                mode = (CODING_MODE)(3 + VP6_DecodeBool(&pbi->br, Stats[4]));
            } else {
                mode = (CODING_MODE)(2 * VP6_DecodeBool(&pbi->br, Stats[3]));
            }
        }
    }

    return mode;
}

void VP6_decodeModeAndMotionVector(struct PB_INSTANCE *pbi, unsigned int MBrow, unsigned int MBcol) {
    int type;
    int x;
    int y;
    unsigned int k;
    CODING_MODE mode;
    MOTION_VECTOR mv;

    VP6_FindNearestandNextNearest(pbi, MBrow, MBcol, 1, &type);

    mode = VP6_DecodeMode(pbi, pbi->LastMode, type);
    pbi->LastMode = mode;

    pbi->predictionMode[MBrow * pbi->MBCols + MBcol] = mode;
    pbi->mbi.Mode = mode;

    if (mode == CODE_INTER_FOURMV) {
        pbi->mbi.BlockMode[0] = VP6_DecodeBlockMode(pbi);
        pbi->mbi.BlockMode[1] = VP6_DecodeBlockMode(pbi);
        pbi->mbi.BlockMode[2] = VP6_DecodeBlockMode(pbi);
        pbi->mbi.BlockMode[3] = VP6_DecodeBlockMode(pbi);

        pbi->mbi.BlockMode[4] = mode;
        pbi->mbi.BlockMode[5] = mode;
        x = 0;
        y = 0;

        for (k = 0; k < 4; k++) {
            if (pbi->mbi.BlockMode[k] == CODE_INTER_NO_MV) {
                pbi->mbi.Mv[k].x = 0;
                pbi->mbi.Mv[k].y = 0;
            } else if (pbi->mbi.BlockMode[k] == CODE_INTER_NEAREST_MV) {
                pbi->mbi.Mv[k].x = pbi->mbi.NearestInterMVect.x;
                pbi->mbi.Mv[k].y = pbi->mbi.NearestInterMVect.y;
                x += pbi->mbi.NearestInterMVect.x;
                y += pbi->mbi.NearestInterMVect.y;
            } else if (pbi->mbi.BlockMode[k] == CODE_INTER_NEAR_MV) {
                pbi->mbi.Mv[k].x = pbi->mbi.NearInterMVect.x;
                pbi->mbi.Mv[k].y = pbi->mbi.NearInterMVect.y;
                x += pbi->mbi.NearInterMVect.x;
                y += pbi->mbi.NearInterMVect.y;
            } else if (pbi->mbi.BlockMode[k] == CODE_INTER_PLUS_MV) {
                VP6_decodeMotionVector(pbi, &mv, CODE_INTER_PLUS_MV);
                pbi->mbi.Mv[k].x = mv.x;
                pbi->mbi.Mv[k].y = mv.y;
                x += mv.x;
                y += mv.y;
            }
        }

        { int t = x + 1; int u; if (x >= 0) { u = (t + 1) >> 2; } else { u = t >> 2; } x = u; }
        { int t = y + 1; int u; if (y >= 0) { u = (t + 1) >> 2; } else { u = t >> 2; } y = u; }

        pbi->MBMotionVector[MBrow * pbi->MBCols + MBcol].x = pbi->mbi.Mv[3].x;
        pbi->MBMotionVector[MBrow * pbi->MBCols + MBcol].y = pbi->mbi.Mv[3].y;

        pbi->mbi.Mv[4].x = x;
        pbi->mbi.Mv[4].y = y;

        pbi->mbi.Mv[5].x = x;
        pbi->mbi.Mv[5].y = y;
    } else {
        switch (mode) {
        case CODE_INTER_NEAREST_MV:
            x = pbi->mbi.NearestInterMVect.x;
            y = pbi->mbi.NearestInterMVect.y;
            break;
        case CODE_INTER_NEAR_MV:
            x = pbi->mbi.NearInterMVect.x;
            y = pbi->mbi.NearInterMVect.y;
            break;
        case CODE_GOLD_NEAREST_MV:
            VP6_FindNearestandNextNearest(pbi, MBrow, MBcol, 2, &type);
            x = pbi->mbi.NearestGoldMVect.x;
            y = pbi->mbi.NearestGoldMVect.y;
            break;
        case CODE_GOLD_NEAR_MV:
            VP6_FindNearestandNextNearest(pbi, MBrow, MBcol, 2, &type);
            x = pbi->mbi.NearGoldMVect.x;
            y = pbi->mbi.NearGoldMVect.y;
            break;
        case CODE_INTER_PLUS_MV:
            VP6_decodeMotionVector(pbi, &mv, CODE_INTER_PLUS_MV);
            x = mv.x;
            y = mv.y;
            break;
        case CODE_GOLDEN_MV:
            VP6_FindNearestandNextNearest(pbi, MBrow, MBcol, 2, &type);
            VP6_decodeMotionVector(pbi, &mv, CODE_GOLDEN_MV);
            x = mv.x;
            y = mv.y;
            break;
        default:
            x = 0;
            y = 0;
            break;
        }

        pbi->MBMotionVector[MBrow * pbi->MBCols + MBcol].x = x;
        pbi->MBMotionVector[MBrow * pbi->MBCols + MBcol].y = y;

        for (k = 0; k < 6; k++) {
            pbi->mbi.Mv[k].x = x;
            pbi->mbi.Mv[k].y = y;
            pbi->mbi.BlockMode[k] = mode;
        }
    }
}
