# decodexac en ensamblador de EA para PS2 (ronda 18).
#
# En PS2 el bucle de decodexac esta desenrollado 14 veces con adda.s/madda.s/madd.s
# ($f10/$f11 de estado, $f0/$f1 de constantes; bytes psrc[1..14], stores pdst[0..108]),
# con tres etiquetas locales con nombre en el symtab de la A124 (decodeblock en 0x4f2fd0,
# decodeps2normal en 0x4f3060, done en 0x4f3460) y marco a mano (addi sp, lq ra).
# ee-gcc 2.9-ee-991111 no tiene patron para madda.s/madd.s/adda.s: es asm de EA por
# compilador, no por forma (el C pierde 840 B por PS2; -ffast-math medido neutro).
# El marco a mano no sale de C + asm en linea (el compilador emite addiu/lw), asi que
# esta TU va en .s solo para PS2 (GC sigue con eaxadecf.cpp). Mismo camino que
# LibSN/metrotrk.s para codigo que nunca fue C.
#
# La A124 y SLUS son instruccion por instruccion iguales salvo 5 palabras reubicables
# (xafilterf, jal process_raw_block, 2x j: aqui simbolicas, el enlazador pone cada una).
# Los comentarios /* fileoff vaddr bytes */ son de la A124. Las 7 funciones vecinas ya
# estaban al 100 % desde C en las dos PS2; aqui van transcritas tal cual.
#
# xatablef (global en el original, sin TU conocida en el mapa) la define esta TU en
# snd 9.06.00 (valores de eaxadecf.cpp:57-72, igual que enlaza hoy PS2).

.include "macro.inc"

.set noat
.set noreorder

.include "macro.inc"

.set noat
.set noreorder

.section .text, "ax"


.align 3

glabel process_raw_block
    /* 3F3ED0 004F2ED0 2D388000 */  daddu      $a3, $a0, $zero
    /* 3F3ED4 004F2ED4 0C00E68C */  lw         $a2, 0xC($a3)
    /* 3F3ED8 004F2ED8 EE000324 */  addiu      $v1, $zero, 0xEE
    /* 3F3EDC 004F2EDC 0000C290 */  lbu        $v0, 0x0($a2)
    /* 3F3EE0 004F2EE0 2C004314 */  bne        $v0, $v1, .L_eax_001
    /* 3F3EE4 004F2EE4 F0FFBD27 */   addiu     $sp, $sp, -0x10
    /* 3F3EE8 004F2EE8 0100C324 */  addiu      $v1, $a2, 0x1
    /* 3F3EEC 004F2EEC 0300C424 */  addiu      $a0, $a2, 0x3
    /* 3F3EF0 004F2EF0 0C00E3AC */  sw         $v1, 0xC($a3)
    /* 3F3EF4 004F2EF4 0500C524 */  addiu      $a1, $a2, 0x5
    /* 3F3EF8 004F2EF8 2D400000 */  daddu      $t0, $zero, $zero
    /* 3F3EFC 004F2EFC 01006290 */  lbu        $v0, 0x1($v1)
    /* 3F3F00 004F2F00 0000A2A3 */  sb         $v0, 0x0($sp)
    /* 3F3F04 004F2F04 0100C390 */  lbu        $v1, 0x1($a2)
    /* 3F3F08 004F2F08 0C00E4AC */  sw         $a0, 0xC($a3)
    /* 3F3F0C 004F2F0C 0100A3A3 */  sb         $v1, 0x1($sp)
    /* 3F3F10 004F2F10 0000A287 */  lh         $v0, 0x0($sp)
    /* 3F3F14 004F2F14 00008244 */  mtc1       $v0, $f0
    /* 3F3F18 004F2F18 20008046 */  cvt.s.w    $f0, $f0
    /* 3F3F1C 004F2F1C 0400E0E4 */  swc1       $f0, 0x4($a3)
    /* 3F3F20 004F2F20 01008290 */  lbu        $v0, 0x1($a0)
    /* 3F3F24 004F2F24 0000A2A3 */  sb         $v0, 0x0($sp)
    /* 3F3F28 004F2F28 0300C390 */  lbu        $v1, 0x3($a2)
    /* 3F3F2C 004F2F2C 0C00E5AC */  sw         $a1, 0xC($a3)
    /* 3F3F30 004F2F30 0100A3A3 */  sb         $v1, 0x1($sp)
    /* 3F3F34 004F2F34 0000A287 */  lh         $v0, 0x0($sp)
    /* 3F3F38 004F2F38 00008244 */  mtc1       $v0, $f0
    /* 3F3F3C 004F2F3C 20008046 */  cvt.s.w    $f0, $f0
    /* 3F3F40 004F2F40 0800E0E4 */  swc1       $f0, 0x8($a3)
    /* 3F3F44 004F2F44 00000000 */  nop
.align 2
.L_eax_002:
    /* 3F3F48 004F2F48 0C00E48C */  lw         $a0, 0xC($a3)
    /* 3F3F4C 004F2F4C 01000825 */  addiu      $t0, $t0, 0x1
    /* 3F3F50 004F2F50 1000E58C */  lw         $a1, 0x10($a3)
    /* 3F3F54 004F2F54 1C00062D */  sltiu      $a2, $t0, 0x1C
    /* 3F3F58 004F2F58 01008290 */  lbu        $v0, 0x1($a0)
    /* 3F3F5C 004F2F5C 0000A2A3 */  sb         $v0, 0x0($sp)
    /* 3F3F60 004F2F60 00008390 */  lbu        $v1, 0x0($a0)
    /* 3F3F64 004F2F64 0100A3A3 */  sb         $v1, 0x1($sp)
    /* 3F3F68 004F2F68 0000A287 */  lh         $v0, 0x0($sp)
    /* 3F3F6C 004F2F6C 00008244 */  mtc1       $v0, $f0
    /* 3F3F70 004F2F70 20008046 */  cvt.s.w    $f0, $f0
    /* 3F3F74 004F2F74 0000A0E4 */  swc1       $f0, 0x0($a1)
    /* 3F3F78 004F2F78 1000E28C */  lw         $v0, 0x10($a3)
    /* 3F3F7C 004F2F7C 0C00E38C */  lw         $v1, 0xC($a3)
    /* 3F3F80 004F2F80 04004224 */  addiu      $v0, $v0, 0x4
    /* 3F3F84 004F2F84 02006324 */  addiu      $v1, $v1, 0x2
    /* 3F3F88 004F2F88 1000E2AC */  sw         $v0, 0x10($a3)
    /* 3F3F8C 004F2F8C EEFFC014 */  bnez       $a2, .L_eax_002
    /* 3F3F90 004F2F90 0C00E3AC */   sw        $v1, 0xC($a3)
.align 2
.L_eax_001:
    /* 3F3F94 004F2F94 0800E003 */  jr         $ra
    /* 3F3F98 004F2F98 1000BD27 */   addiu     $sp, $sp, 0x10
endlabel process_raw_block
    /* 3F3F9C 004F2F9C 00000000 */  nop

.align 3
/* Handwritten function */

glabel decodexac__3SndPQ23Snd10MXAPACKETF
    /* 3F3FA0 004F2FA0 6500023C */  lui        $v0, %hi(_3Snd$xafilterf)
    /* 3F3FA4 004F2FA4 2D188000 */  daddu      $v1, $a0, $zero
    /* 3F3FA8 004F2FA8 28DD4224 */  addiu      $v0, $v0, %lo(_3Snd$xafilterf)
    /* 3F3FAC 004F2FAC 2D206000 */  daddu      $a0, $v1, $zero
    /* 3F3FB0 004F2FB0 2D284000 */  daddu      $a1, $v0, $zero
    /* 3F3FB4 004F2FB4 F0FFBD23 */  addi       $sp, $sp, -0x10 /* handwritten instruction */
    /* 3F3FB8 004F2FB8 0000BFAF */  sw         $ra, 0x0($sp)
    /* 3F3FBC 004F2FBC 0000868C */  lw         $a2, 0x0($a0)
    /* 3F3FC0 004F2FC0 04008AC4 */  lwc1       $f10, 0x4($a0)
    /* 3F3FC4 004F2FC4 08008BC4 */  lwc1       $f11, 0x8($a0)
    /* 3F3FC8 004F2FC8 0C00878C */  lw         $a3, 0xC($a0)
    /* 3F3FCC 004F2FCC 1000888C */  lw         $t0, 0x10($a0)
.align 2
decodeblock:
    /* 3F3FD0 004F2FD0 2301C018 */  blez       $a2, done
    /* 3F3FD4 004F2FD4 00000000 */   nop
    /* 3F3FD8 004F2FD8 0000E990 */  lbu        $t1, 0x0($a3)
    /* 3F3FDC 004F2FDC E4FFC620 */  addi       $a2, $a2, -0x1C /* handwritten instruction */
    /* 3F3FE0 004F2FE0 EE000D24 */  addiu      $t5, $zero, 0xEE
    /* 3F3FE4 004F2FE4 0000EE90 */  lbu        $t6, 0x0($a3)
    /* 3F3FE8 004F2FE8 1D00AE15 */  bne        $t5, $t6, decodeps2normal
    /* 3F3FEC 004F2FEC 00000000 */   nop
    /* 3F3FF0 004F2FF0 D0FFBD23 */  addi       $sp, $sp, -0x30 /* handwritten instruction */
    /* 3F3FF4 004F2FF4 2000A4AF */  sw         $a0, 0x20($sp)
    /* 3F3FF8 004F2FF8 1C00A5AF */  sw         $a1, 0x1C($sp)
    /* 3F3FFC 004F2FFC 1800A6AF */  sw         $a2, 0x18($sp)
    /* 3F4000 004F3000 1400A7AF */  sw         $a3, 0x14($sp)
    /* 3F4004 004F3004 1000A8AF */  sw         $t0, 0x10($sp)
    /* 3F4008 004F3008 0C00A9AF */  sw         $t1, 0xC($sp)
    /* 3F400C 004F300C 0800AAAF */  sw         $t2, 0x8($sp)
    /* 3F4010 004F3010 0400ABAF */  sw         $t3, 0x4($sp)
    /* 3F4014 004F3014 0000ACAF */  sw         $t4, 0x0($sp)
    /* 3F4018 004F3018 B4CB130C */  jal        process_raw_block
    /* 3F401C 004F301C 00000000 */   nop
    /* 3F4020 004F3020 0000AC8F */  lw         $t4, 0x0($sp)
    /* 3F4024 004F3024 0400AB8F */  lw         $t3, 0x4($sp)
    /* 3F4028 004F3028 0800AA8F */  lw         $t2, 0x8($sp)
    /* 3F402C 004F302C 0C00A98F */  lw         $t1, 0xC($sp)
    /* 3F4030 004F3030 1000A88F */  lw         $t0, 0x10($sp)
    /* 3F4034 004F3034 1400A78F */  lw         $a3, 0x14($sp)
    /* 3F4038 004F3038 1800A68F */  lw         $a2, 0x18($sp)
    /* 3F403C 004F303C 1C00A58F */  lw         $a1, 0x1C($sp)
    /* 3F4040 004F3040 2000A48F */  lw         $a0, 0x20($sp)
    /* 3F4044 004F3044 3000BD23 */  addi       $sp, $sp, 0x30 /* handwritten instruction */
    /* 3F4048 004F3048 04008AC4 */  lwc1       $f10, 0x4($a0)
    /* 3F404C 004F304C 08008BC4 */  lwc1       $f11, 0x8($a0)
    /* 3F4050 004F3050 0C00878C */  lw         $a3, 0xC($a0)
    /* 3F4054 004F3054 1000888C */  lw         $t0, 0x10($a0)
    /* 3F4058 004F3058 F4CB1308 */  j          decodeblock
    /* 3F405C 004F305C 00000000 */   nop
.align 2
decodeps2normal:
    /* 3F4060 004F3060 02510900 */  srl        $t2, $t1, 4
    /* 3F4064 004F3064 0F002931 */  andi       $t1, $t1, 0xF
    /* 3F4068 004F3068 80500A00 */  sll        $t2, $t2, 2
    /* 3F406C 004F306C 20504501 */  add        $t2, $t2, $a1 /* handwritten instruction */
    /* 3F4070 004F3070 000040C5 */  lwc1       $f0, 0x0($t2)
    /* 3F4074 004F3074 100041C5 */  lwc1       $f1, 0x10($t2)
    /* 3F4078 004F3078 0C000A20 */  addi       $t2, $zero, 0xC /* handwritten instruction */
    /* 3F407C 004F307C 22504901 */  sub        $t2, $t2, $t1 /* handwritten instruction */
    /* 3F4080 004F3080 10002921 */  addi       $t1, $t1, 0x10 /* handwritten instruction */
    /* 3F4084 004F3084 0100EB80 */  lb         $t3, 0x1($a3)
    /* 3F4088 004F3088 03610B00 */  sra        $t4, $t3, 4
    /* 3F408C 004F308C 005F0B00 */  sll        $t3, $t3, 28
    /* 3F4090 004F3090 04604C01 */  sllv       $t4, $t4, $t2
    /* 3F4094 004F3094 07582B01 */  srav       $t3, $t3, $t1
    /* 3F4098 004F3098 00108C44 */  mtc1       $t4, $f2
    /* 3F409C 004F309C 00188B44 */  mtc1       $t3, $f3
    /* 3F40A0 004F30A0 A0108046 */  cvt.s.w    $f2, $f2
    /* 3F40A4 004F30A4 E0188046 */  cvt.s.w    $f3, $f3
    /* 3F40A8 004F30A8 00F88044 */  mtc1       $zero, $f31
    /* 3F40AC 004F30AC 18101F46 */  adda.s     $f2, $f31
    /* 3F40B0 004F30B0 1E500046 */  madda.s    $f10, $f0
    /* 3F40B4 004F30B4 DC5A0146 */  madd.s     $f11, $f11, $f1
    /* 3F40B8 004F30B8 0200EB80 */  lb         $t3, 0x2($a3)
    /* 3F40BC 004F30BC 03610B00 */  sra        $t4, $t3, 4
    /* 3F40C0 004F30C0 005F0B00 */  sll        $t3, $t3, 28
    /* 3F40C4 004F30C4 04604C01 */  sllv       $t4, $t4, $t2
    /* 3F40C8 004F30C8 07582B01 */  srav       $t3, $t3, $t1
    /* 3F40CC 004F30CC 00000BE5 */  swc1       $f11, 0x0($t0)
    /* 3F40D0 004F30D0 18181F46 */  adda.s     $f3, $f31
    /* 3F40D4 004F30D4 1E580046 */  madda.s    $f11, $f0
    /* 3F40D8 004F30D8 9C520146 */  madd.s     $f10, $f10, $f1
    /* 3F40DC 004F30DC 00208C44 */  mtc1       $t4, $f4
    /* 3F40E0 004F30E0 00288B44 */  mtc1       $t3, $f5
    /* 3F40E4 004F30E4 20218046 */  cvt.s.w    $f4, $f4
    /* 3F40E8 004F30E8 60298046 */  cvt.s.w    $f5, $f5
    /* 3F40EC 004F30EC 04000AE5 */  swc1       $f10, 0x4($t0)
    /* 3F40F0 004F30F0 18201F46 */  adda.s     $f4, $f31
    /* 3F40F4 004F30F4 1E500046 */  madda.s    $f10, $f0
    /* 3F40F8 004F30F8 DC5A0146 */  madd.s     $f11, $f11, $f1
    /* 3F40FC 004F30FC 0300EB80 */  lb         $t3, 0x3($a3)
    /* 3F4100 004F3100 03610B00 */  sra        $t4, $t3, 4
    /* 3F4104 004F3104 005F0B00 */  sll        $t3, $t3, 28
    /* 3F4108 004F3108 04604C01 */  sllv       $t4, $t4, $t2
    /* 3F410C 004F310C 07582B01 */  srav       $t3, $t3, $t1
    /* 3F4110 004F3110 08000BE5 */  swc1       $f11, 0x8($t0)
    /* 3F4114 004F3114 18281F46 */  adda.s     $f5, $f31
    /* 3F4118 004F3118 1E580046 */  madda.s    $f11, $f0
    /* 3F411C 004F311C 9C520146 */  madd.s     $f10, $f10, $f1
    /* 3F4120 004F3120 00108C44 */  mtc1       $t4, $f2
    /* 3F4124 004F3124 00188B44 */  mtc1       $t3, $f3
    /* 3F4128 004F3128 A0108046 */  cvt.s.w    $f2, $f2
    /* 3F412C 004F312C E0188046 */  cvt.s.w    $f3, $f3
    /* 3F4130 004F3130 0C000AE5 */  swc1       $f10, 0xC($t0)
    /* 3F4134 004F3134 18101F46 */  adda.s     $f2, $f31
    /* 3F4138 004F3138 1E500046 */  madda.s    $f10, $f0
    /* 3F413C 004F313C DC5A0146 */  madd.s     $f11, $f11, $f1
    /* 3F4140 004F3140 0400EB80 */  lb         $t3, 0x4($a3)
    /* 3F4144 004F3144 03610B00 */  sra        $t4, $t3, 4
    /* 3F4148 004F3148 005F0B00 */  sll        $t3, $t3, 28
    /* 3F414C 004F314C 04604C01 */  sllv       $t4, $t4, $t2
    /* 3F4150 004F3150 07582B01 */  srav       $t3, $t3, $t1
    /* 3F4154 004F3154 10000BE5 */  swc1       $f11, 0x10($t0)
    /* 3F4158 004F3158 18181F46 */  adda.s     $f3, $f31
    /* 3F415C 004F315C 1E580046 */  madda.s    $f11, $f0
    /* 3F4160 004F3160 9C520146 */  madd.s     $f10, $f10, $f1
    /* 3F4164 004F3164 00208C44 */  mtc1       $t4, $f4
    /* 3F4168 004F3168 00288B44 */  mtc1       $t3, $f5
    /* 3F416C 004F316C 20218046 */  cvt.s.w    $f4, $f4
    /* 3F4170 004F3170 60298046 */  cvt.s.w    $f5, $f5
    /* 3F4174 004F3174 14000AE5 */  swc1       $f10, 0x14($t0)
    /* 3F4178 004F3178 18201F46 */  adda.s     $f4, $f31
    /* 3F417C 004F317C 1E500046 */  madda.s    $f10, $f0
    /* 3F4180 004F3180 DC5A0146 */  madd.s     $f11, $f11, $f1
    /* 3F4184 004F3184 0500EB80 */  lb         $t3, 0x5($a3)
    /* 3F4188 004F3188 03610B00 */  sra        $t4, $t3, 4
    /* 3F418C 004F318C 005F0B00 */  sll        $t3, $t3, 28
    /* 3F4190 004F3190 04604C01 */  sllv       $t4, $t4, $t2
    /* 3F4194 004F3194 07582B01 */  srav       $t3, $t3, $t1
    /* 3F4198 004F3198 18000BE5 */  swc1       $f11, 0x18($t0)
    /* 3F419C 004F319C 18281F46 */  adda.s     $f5, $f31
    /* 3F41A0 004F31A0 1E580046 */  madda.s    $f11, $f0
    /* 3F41A4 004F31A4 9C520146 */  madd.s     $f10, $f10, $f1
    /* 3F41A8 004F31A8 00108C44 */  mtc1       $t4, $f2
    /* 3F41AC 004F31AC 00188B44 */  mtc1       $t3, $f3
    /* 3F41B0 004F31B0 A0108046 */  cvt.s.w    $f2, $f2
    /* 3F41B4 004F31B4 E0188046 */  cvt.s.w    $f3, $f3
    /* 3F41B8 004F31B8 1C000AE5 */  swc1       $f10, 0x1C($t0)
    /* 3F41BC 004F31BC 18101F46 */  adda.s     $f2, $f31
    /* 3F41C0 004F31C0 1E500046 */  madda.s    $f10, $f0
    /* 3F41C4 004F31C4 DC5A0146 */  madd.s     $f11, $f11, $f1
    /* 3F41C8 004F31C8 0600EB80 */  lb         $t3, 0x6($a3)
    /* 3F41CC 004F31CC 03610B00 */  sra        $t4, $t3, 4
    /* 3F41D0 004F31D0 005F0B00 */  sll        $t3, $t3, 28
    /* 3F41D4 004F31D4 04604C01 */  sllv       $t4, $t4, $t2
    /* 3F41D8 004F31D8 07582B01 */  srav       $t3, $t3, $t1
    /* 3F41DC 004F31DC 20000BE5 */  swc1       $f11, 0x20($t0)
    /* 3F41E0 004F31E0 18181F46 */  adda.s     $f3, $f31
    /* 3F41E4 004F31E4 1E580046 */  madda.s    $f11, $f0
    /* 3F41E8 004F31E8 9C520146 */  madd.s     $f10, $f10, $f1
    /* 3F41EC 004F31EC 00208C44 */  mtc1       $t4, $f4
    /* 3F41F0 004F31F0 00288B44 */  mtc1       $t3, $f5
    /* 3F41F4 004F31F4 20218046 */  cvt.s.w    $f4, $f4
    /* 3F41F8 004F31F8 60298046 */  cvt.s.w    $f5, $f5
    /* 3F41FC 004F31FC 24000AE5 */  swc1       $f10, 0x24($t0)
    /* 3F4200 004F3200 18201F46 */  adda.s     $f4, $f31
    /* 3F4204 004F3204 1E500046 */  madda.s    $f10, $f0
    /* 3F4208 004F3208 DC5A0146 */  madd.s     $f11, $f11, $f1
    /* 3F420C 004F320C 0700EB80 */  lb         $t3, 0x7($a3)
    /* 3F4210 004F3210 03610B00 */  sra        $t4, $t3, 4
    /* 3F4214 004F3214 005F0B00 */  sll        $t3, $t3, 28
    /* 3F4218 004F3218 04604C01 */  sllv       $t4, $t4, $t2
    /* 3F421C 004F321C 07582B01 */  srav       $t3, $t3, $t1
    /* 3F4220 004F3220 28000BE5 */  swc1       $f11, 0x28($t0)
    /* 3F4224 004F3224 18281F46 */  adda.s     $f5, $f31
    /* 3F4228 004F3228 1E580046 */  madda.s    $f11, $f0
    /* 3F422C 004F322C 9C520146 */  madd.s     $f10, $f10, $f1
    /* 3F4230 004F3230 00108C44 */  mtc1       $t4, $f2
    /* 3F4234 004F3234 00188B44 */  mtc1       $t3, $f3
    /* 3F4238 004F3238 A0108046 */  cvt.s.w    $f2, $f2
    /* 3F423C 004F323C E0188046 */  cvt.s.w    $f3, $f3
    /* 3F4240 004F3240 2C000AE5 */  swc1       $f10, 0x2C($t0)
    /* 3F4244 004F3244 18101F46 */  adda.s     $f2, $f31
    /* 3F4248 004F3248 1E500046 */  madda.s    $f10, $f0
    /* 3F424C 004F324C DC5A0146 */  madd.s     $f11, $f11, $f1
    /* 3F4250 004F3250 0800EB80 */  lb         $t3, 0x8($a3)
    /* 3F4254 004F3254 03610B00 */  sra        $t4, $t3, 4
    /* 3F4258 004F3258 005F0B00 */  sll        $t3, $t3, 28
    /* 3F425C 004F325C 04604C01 */  sllv       $t4, $t4, $t2
    /* 3F4260 004F3260 07582B01 */  srav       $t3, $t3, $t1
    /* 3F4264 004F3264 30000BE5 */  swc1       $f11, 0x30($t0)
    /* 3F4268 004F3268 18181F46 */  adda.s     $f3, $f31
    /* 3F426C 004F326C 1E580046 */  madda.s    $f11, $f0
    /* 3F4270 004F3270 9C520146 */  madd.s     $f10, $f10, $f1
    /* 3F4274 004F3274 00208C44 */  mtc1       $t4, $f4
    /* 3F4278 004F3278 00288B44 */  mtc1       $t3, $f5
    /* 3F427C 004F327C 20218046 */  cvt.s.w    $f4, $f4
    /* 3F4280 004F3280 60298046 */  cvt.s.w    $f5, $f5
    /* 3F4284 004F3284 34000AE5 */  swc1       $f10, 0x34($t0)
    /* 3F4288 004F3288 18201F46 */  adda.s     $f4, $f31
    /* 3F428C 004F328C 1E500046 */  madda.s    $f10, $f0
    /* 3F4290 004F3290 DC5A0146 */  madd.s     $f11, $f11, $f1
    /* 3F4294 004F3294 0900EB80 */  lb         $t3, 0x9($a3)
    /* 3F4298 004F3298 03610B00 */  sra        $t4, $t3, 4
    /* 3F429C 004F329C 005F0B00 */  sll        $t3, $t3, 28
    /* 3F42A0 004F32A0 04604C01 */  sllv       $t4, $t4, $t2
    /* 3F42A4 004F32A4 07582B01 */  srav       $t3, $t3, $t1
    /* 3F42A8 004F32A8 38000BE5 */  swc1       $f11, 0x38($t0)
    /* 3F42AC 004F32AC 18281F46 */  adda.s     $f5, $f31
    /* 3F42B0 004F32B0 1E580046 */  madda.s    $f11, $f0
    /* 3F42B4 004F32B4 9C520146 */  madd.s     $f10, $f10, $f1
    /* 3F42B8 004F32B8 00108C44 */  mtc1       $t4, $f2
    /* 3F42BC 004F32BC 00188B44 */  mtc1       $t3, $f3
    /* 3F42C0 004F32C0 A0108046 */  cvt.s.w    $f2, $f2
    /* 3F42C4 004F32C4 E0188046 */  cvt.s.w    $f3, $f3
    /* 3F42C8 004F32C8 3C000AE5 */  swc1       $f10, 0x3C($t0)
    /* 3F42CC 004F32CC 18101F46 */  adda.s     $f2, $f31
    /* 3F42D0 004F32D0 1E500046 */  madda.s    $f10, $f0
    /* 3F42D4 004F32D4 DC5A0146 */  madd.s     $f11, $f11, $f1
    /* 3F42D8 004F32D8 0A00EB80 */  lb         $t3, 0xA($a3)
    /* 3F42DC 004F32DC 03610B00 */  sra        $t4, $t3, 4
    /* 3F42E0 004F32E0 005F0B00 */  sll        $t3, $t3, 28
    /* 3F42E4 004F32E4 04604C01 */  sllv       $t4, $t4, $t2
    /* 3F42E8 004F32E8 07582B01 */  srav       $t3, $t3, $t1
    /* 3F42EC 004F32EC 40000BE5 */  swc1       $f11, 0x40($t0)
    /* 3F42F0 004F32F0 18181F46 */  adda.s     $f3, $f31
    /* 3F42F4 004F32F4 1E580046 */  madda.s    $f11, $f0
    /* 3F42F8 004F32F8 9C520146 */  madd.s     $f10, $f10, $f1
    /* 3F42FC 004F32FC 00208C44 */  mtc1       $t4, $f4
    /* 3F4300 004F3300 00288B44 */  mtc1       $t3, $f5
    /* 3F4304 004F3304 20218046 */  cvt.s.w    $f4, $f4
    /* 3F4308 004F3308 60298046 */  cvt.s.w    $f5, $f5
    /* 3F430C 004F330C 44000AE5 */  swc1       $f10, 0x44($t0)
    /* 3F4310 004F3310 18201F46 */  adda.s     $f4, $f31
    /* 3F4314 004F3314 1E500046 */  madda.s    $f10, $f0
    /* 3F4318 004F3318 DC5A0146 */  madd.s     $f11, $f11, $f1
    /* 3F431C 004F331C 0B00EB80 */  lb         $t3, 0xB($a3)
    /* 3F4320 004F3320 03610B00 */  sra        $t4, $t3, 4
    /* 3F4324 004F3324 005F0B00 */  sll        $t3, $t3, 28
    /* 3F4328 004F3328 04604C01 */  sllv       $t4, $t4, $t2
    /* 3F432C 004F332C 07582B01 */  srav       $t3, $t3, $t1
    /* 3F4330 004F3330 48000BE5 */  swc1       $f11, 0x48($t0)
    /* 3F4334 004F3334 18281F46 */  adda.s     $f5, $f31
    /* 3F4338 004F3338 1E580046 */  madda.s    $f11, $f0
    /* 3F433C 004F333C 9C520146 */  madd.s     $f10, $f10, $f1
    /* 3F4340 004F3340 00108C44 */  mtc1       $t4, $f2
    /* 3F4344 004F3344 00188B44 */  mtc1       $t3, $f3
    /* 3F4348 004F3348 A0108046 */  cvt.s.w    $f2, $f2
    /* 3F434C 004F334C E0188046 */  cvt.s.w    $f3, $f3
    /* 3F4350 004F3350 4C000AE5 */  swc1       $f10, 0x4C($t0)
    /* 3F4354 004F3354 18101F46 */  adda.s     $f2, $f31
    /* 3F4358 004F3358 1E500046 */  madda.s    $f10, $f0
    /* 3F435C 004F335C DC5A0146 */  madd.s     $f11, $f11, $f1
    /* 3F4360 004F3360 0C00EB80 */  lb         $t3, 0xC($a3)
    /* 3F4364 004F3364 03610B00 */  sra        $t4, $t3, 4
    /* 3F4368 004F3368 005F0B00 */  sll        $t3, $t3, 28
    /* 3F436C 004F336C 04604C01 */  sllv       $t4, $t4, $t2
    /* 3F4370 004F3370 07582B01 */  srav       $t3, $t3, $t1
    /* 3F4374 004F3374 50000BE5 */  swc1       $f11, 0x50($t0)
    /* 3F4378 004F3378 18181F46 */  adda.s     $f3, $f31
    /* 3F437C 004F337C 1E580046 */  madda.s    $f11, $f0
    /* 3F4380 004F3380 9C520146 */  madd.s     $f10, $f10, $f1
    /* 3F4384 004F3384 00208C44 */  mtc1       $t4, $f4
    /* 3F4388 004F3388 00288B44 */  mtc1       $t3, $f5
    /* 3F438C 004F338C 20218046 */  cvt.s.w    $f4, $f4
    /* 3F4390 004F3390 60298046 */  cvt.s.w    $f5, $f5
    /* 3F4394 004F3394 54000AE5 */  swc1       $f10, 0x54($t0)
    /* 3F4398 004F3398 18201F46 */  adda.s     $f4, $f31
    /* 3F439C 004F339C 1E500046 */  madda.s    $f10, $f0
    /* 3F43A0 004F33A0 DC5A0146 */  madd.s     $f11, $f11, $f1
    /* 3F43A4 004F33A4 0D00EB80 */  lb         $t3, 0xD($a3)
    /* 3F43A8 004F33A8 03610B00 */  sra        $t4, $t3, 4
    /* 3F43AC 004F33AC 005F0B00 */  sll        $t3, $t3, 28
    /* 3F43B0 004F33B0 04604C01 */  sllv       $t4, $t4, $t2
    /* 3F43B4 004F33B4 07582B01 */  srav       $t3, $t3, $t1
    /* 3F43B8 004F33B8 58000BE5 */  swc1       $f11, 0x58($t0)
    /* 3F43BC 004F33BC 18281F46 */  adda.s     $f5, $f31
    /* 3F43C0 004F33C0 1E580046 */  madda.s    $f11, $f0
    /* 3F43C4 004F33C4 9C520146 */  madd.s     $f10, $f10, $f1
    /* 3F43C8 004F33C8 00108C44 */  mtc1       $t4, $f2
    /* 3F43CC 004F33CC 00188B44 */  mtc1       $t3, $f3
    /* 3F43D0 004F33D0 A0108046 */  cvt.s.w    $f2, $f2
    /* 3F43D4 004F33D4 E0188046 */  cvt.s.w    $f3, $f3
    /* 3F43D8 004F33D8 5C000AE5 */  swc1       $f10, 0x5C($t0)
    /* 3F43DC 004F33DC 18101F46 */  adda.s     $f2, $f31
    /* 3F43E0 004F33E0 1E500046 */  madda.s    $f10, $f0
    /* 3F43E4 004F33E4 DC5A0146 */  madd.s     $f11, $f11, $f1
    /* 3F43E8 004F33E8 0E00EB80 */  lb         $t3, 0xE($a3)
    /* 3F43EC 004F33EC 03610B00 */  sra        $t4, $t3, 4
    /* 3F43F0 004F33F0 005F0B00 */  sll        $t3, $t3, 28
    /* 3F43F4 004F33F4 04604C01 */  sllv       $t4, $t4, $t2
    /* 3F43F8 004F33F8 07582B01 */  srav       $t3, $t3, $t1
    /* 3F43FC 004F33FC 60000BE5 */  swc1       $f11, 0x60($t0)
    /* 3F4400 004F3400 18181F46 */  adda.s     $f3, $f31
    /* 3F4404 004F3404 1E580046 */  madda.s    $f11, $f0
    /* 3F4408 004F3408 9C520146 */  madd.s     $f10, $f10, $f1
    /* 3F440C 004F340C 00208C44 */  mtc1       $t4, $f4
    /* 3F4410 004F3410 00288B44 */  mtc1       $t3, $f5
    /* 3F4414 004F3414 20218046 */  cvt.s.w    $f4, $f4
    /* 3F4418 004F3418 60298046 */  cvt.s.w    $f5, $f5
    /* 3F441C 004F341C 64000AE5 */  swc1       $f10, 0x64($t0)
    /* 3F4420 004F3420 18201F46 */  adda.s     $f4, $f31
    /* 3F4424 004F3424 1E500046 */  madda.s    $f10, $f0
    /* 3F4428 004F3428 DC5A0146 */  madd.s     $f11, $f11, $f1
    /* 3F442C 004F342C 68000BE5 */  swc1       $f11, 0x68($t0)
    /* 3F4430 004F3430 18281F46 */  adda.s     $f5, $f31
    /* 3F4434 004F3434 1E580046 */  madda.s    $f11, $f0
    /* 3F4438 004F3438 9C520146 */  madd.s     $f10, $f10, $f1
    /* 3F443C 004F343C 6C000AE5 */  swc1       $f10, 0x6C($t0)
    /* 3F4440 004F3440 0F00E720 */  addi       $a3, $a3, 0xF /* handwritten instruction */
    /* 3F4444 004F3444 70000821 */  addi       $t0, $t0, 0x70 /* handwritten instruction */
    /* 3F4448 004F3448 04008AE4 */  swc1       $f10, 0x4($a0)
    /* 3F444C 004F344C 08008BE4 */  swc1       $f11, 0x8($a0)
    /* 3F4450 004F3450 0C0087AC */  sw         $a3, 0xC($a0)
    /* 3F4454 004F3454 100088AC */  sw         $t0, 0x10($a0)
    /* 3F4458 004F3458 F4CB1308 */  j          decodeblock
    /* 3F445C 004F345C 00000000 */   nop
.align 2
done:
    /* 3F4460 004F3460 000086AC */  sw         $a2, 0x0($a0)
    /* 3F4464 004F3464 04008AE4 */  swc1       $f10, 0x4($a0)
    /* 3F4468 004F3468 08008BE4 */  swc1       $f11, 0x8($a0)
    /* 3F446C 004F346C 0C0087AC */  sw         $a3, 0xC($a0)
    /* 3F4470 004F3470 100088AC */  sw         $t0, 0x10($a0)
    /* 3F4474 004F3474 0000BF7B */  lq         $ra, 0x0($sp)
    /* 3F4478 004F3478 1000BD23 */  addi       $sp, $sp, 0x10 /* handwritten instruction */
    /* 3F447C 004F347C 0800E003 */  jr         $ra
    /* 3F4480 004F3480 00000000 */   nop
endlabel decodexac__3SndPQ23Snd10MXAPACKETF
    /* 3F4484 004F3484 00000000 */  nop

.align 3

glabel __nw__Q23Snd12CEAXABLKDecfUi
    /* 3F4488 004F3488 6800033C */  lui        $v1, %hi(_3Snd$CODANew)
    /* 3F448C 004F348C F0FFBD27 */  addiu      $sp, $sp, -0x10
    /* 3F4490 004F3490 E8F4628C */  lw         $v0, %lo(_3Snd$CODANew)($v1)
    /* 3F4494 004F3494 0000BFFF */  sd         $ra, 0x0($sp)
    /* 3F4498 004F3498 09F84000 */  jalr       $v0
    /* 3F449C 004F349C 00000000 */   nop
    /* 3F44A0 004F34A0 0000BFDF */  ld         $ra, 0x0($sp)
    /* 3F44A4 004F34A4 0800E003 */  jr         $ra
    /* 3F44A8 004F34A8 1000BD27 */   addiu     $sp, $sp, 0x10
endlabel __nw__Q23Snd12CEAXABLKDecfUi
    /* 3F44AC 004F34AC 00000000 */  nop

.align 3

glabel __dl__Q23Snd12CEAXABLKDecfPv
    /* 3F44B0 004F34B0 6800033C */  lui        $v1, %hi(_3Snd$CODADelete)
    /* 3F44B4 004F34B4 F0FFBD27 */  addiu      $sp, $sp, -0x10
    /* 3F44B8 004F34B8 ECF4628C */  lw         $v0, %lo(_3Snd$CODADelete)($v1)
    /* 3F44BC 004F34BC 0000BFFF */  sd         $ra, 0x0($sp)
    /* 3F44C0 004F34C0 09F84000 */  jalr       $v0
    /* 3F44C4 004F34C4 00000000 */   nop
    /* 3F44C8 004F34C8 0000BFDF */  ld         $ra, 0x0($sp)
    /* 3F44CC 004F34CC 0800E003 */  jr         $ra
    /* 3F44D0 004F34D0 1000BD27 */   addiu     $sp, $sp, 0x10
endlabel __dl__Q23Snd12CEAXABLKDecfPv
    /* 3F44D4 004F34D4 00000000 */  nop

.align 3

glabel __Q23Snd12CEAXABLKDecf
    /* 3F44D8 004F34D8 2D108000 */  daddu      $v0, $a0, $zero
    /* 3F44DC 004F34DC 0C0080AC */  sw         $zero, 0xC($a0)
    /* 3F44E0 004F34E0 080080AC */  sw         $zero, 0x8($a0)
    /* 3F44E4 004F34E4 040080AC */  sw         $zero, 0x4($a0)
    /* 3F44E8 004F34E8 980080AC */  sw         $zero, 0x98($a0)
    /* 3F44EC 004F34EC 0800E003 */  jr         $ra
    /* 3F44F0 004F34F0 9C0080AC */   sw        $zero, 0x9C($a0)
endlabel __Q23Snd12CEAXABLKDecf
    /* 3F44F4 004F34F4 00000000 */  nop

.align 3

glabel Feed__Q23Snd12CEAXABLKDecfPvii
    /* 3F44F8 004F34F8 0300A054 */  bnel       $a1, $zero, .L_eax_003
    /* 3F44FC 004F34FC 0800828C */   lw        $v0, 0x8($a0)
    /* 3F4500 004F3500 0800E003 */  jr         $ra
    /* 3F4504 004F3504 FFFF0224 */   addiu     $v0, $zero, -0x1
.align 2
.L_eax_003:
    /* 3F4508 004F3508 05004014 */  bnez       $v0, .L_eax_004
    /* 3F450C 004F350C 2D100000 */   daddu     $v0, $zero, $zero
    /* 3F4510 004F3510 A00085AC */  sw         $a1, 0xA0($a0)
    /* 3F4514 004F3514 080087AC */  sw         $a3, 0x8($a0)
    /* 3F4518 004F3518 0800E003 */  jr         $ra
    /* 3F451C 004F351C 0C0086AC */   sw        $a2, 0xC($a0)
.align 2
.L_eax_004:
    /* 3F4520 004F3520 0800E003 */  jr         $ra
    /* 3F4524 004F3524 FFFF0224 */   addiu     $v0, $zero, -0x1
endlabel Feed__Q23Snd12CEAXABLKDecfPvii

.align 3

glabel Decode__Q23Snd12CEAXABLKDecfPPfi
    /* 3F4528 004F3528 A0FFBD27 */  addiu      $sp, $sp, -0x60
    /* 3F452C 004F352C 5000B07F */  sq         $s0, 0x50($sp)
    /* 3F4530 004F3530 2000B37F */  sq         $s3, 0x20($sp)
    /* 3F4534 004F3534 2D808000 */  daddu      $s0, $a0, $zero
    /* 3F4538 004F3538 4000B17F */  sq         $s1, 0x40($sp)
    /* 3F453C 004F353C 2D980000 */  daddu      $s3, $zero, $zero
    /* 3F4540 004F3540 3000B27F */  sq         $s2, 0x30($sp)
    /* 3F4544 004F3544 1000B47F */  sq         $s4, 0x10($sp)
    /* 3F4548 004F3548 0000BFFF */  sd         $ra, 0x0($sp)
    /* 3F454C 004F354C 0000A28C */  lw         $v0, 0x0($a1)
    /* 3F4550 004F3550 0800038E */  lw         $v1, 0x8($s0)
    /* 3F4554 004F3554 03006014 */  bnez       $v1, .L_eax_005
    /* 3F4558 004F3558 A40002AE */   sw        $v0, 0xA4($s0)
    /* 3F455C 004F355C 54000010 */  b          .L_eax_006
    /* 3F4560 004F3560 2D100000 */   daddu     $v0, $zero, $zero
.align 2
.L_eax_005:
    /* 3F4564 004F3564 2D286000 */  daddu      $a1, $v1, $zero
    /* 3F4568 004F3568 2A10C500 */  slt        $v0, $a2, $a1
    /* 3F456C 004F356C 0400038E */  lw         $v1, 0x4($s0)
    /* 3F4570 004F3570 1B006010 */  beqz       $v1, .L_eax_007
    /* 3F4574 004F3574 0B28C200 */   movn      $a1, $a2, $v0
    /* 3F4578 004F3578 2A10A300 */  slt        $v0, $a1, $v1
    /* 3F457C 004F357C 2D206000 */  daddu      $a0, $v1, $zero
    /* 3F4580 004F3580 0B20A200 */  movn       $a0, $a1, $v0
    /* 3F4584 004F3584 0E008018 */  blez       $a0, .L_eax_008
    /* 3F4588 004F3588 2D888000 */   daddu     $s1, $a0, $zero
    /* 3F458C 004F358C 00000000 */  nop
.align 2
.L_eax_009:
    /* 3F4590 004F3590 1000028E */  lw         $v0, 0x10($s0)
    /* 3F4594 004F3594 FFFF3126 */  addiu      $s1, $s1, -0x1
    /* 3F4598 004F3598 A400038E */  lw         $v1, 0xA4($s0)
    /* 3F459C 004F359C 000040C4 */  lwc1       $f0, 0x0($v0)
    /* 3F45A0 004F35A0 000060E4 */  swc1       $f0, 0x0($v1)
    /* 3F45A4 004F35A4 A400028E */  lw         $v0, 0xA4($s0)
    /* 3F45A8 004F35A8 1000038E */  lw         $v1, 0x10($s0)
    /* 3F45AC 004F35AC 04004224 */  addiu      $v0, $v0, 0x4
    /* 3F45B0 004F35B0 04006324 */  addiu      $v1, $v1, 0x4
    /* 3F45B4 004F35B4 A40002AE */  sw         $v0, 0xA4($s0)
    /* 3F45B8 004F35B8 F5FF2016 */  bnez       $s1, .L_eax_009
    /* 3F45BC 004F35BC 100003AE */   sw        $v1, 0x10($s0)
.align 2
.L_eax_008:
    /* 3F45C0 004F35C0 0800038E */  lw         $v1, 0x8($s0)
    /* 3F45C4 004F35C4 2D988000 */  daddu      $s3, $a0, $zero
    /* 3F45C8 004F35C8 0400028E */  lw         $v0, 0x4($s0)
    /* 3F45CC 004F35CC 2328A400 */  subu       $a1, $a1, $a0
    /* 3F45D0 004F35D0 23186400 */  subu       $v1, $v1, $a0
    /* 3F45D4 004F35D4 23104400 */  subu       $v0, $v0, $a0
    /* 3F45D8 004F35D8 080003AE */  sw         $v1, 0x8($s0)
    /* 3F45DC 004F35DC 040002AE */  sw         $v0, 0x4($s0)
.align 2
.L_eax_007:
    /* 3F45E0 004F35E0 1C000224 */  addiu      $v0, $zero, 0x1C
    /* 3F45E4 004F35E4 1A00A200 */  div        $zero, $a1, $v0
    /* 3F45E8 004F35E8 01004050 */  beql       $v0, $zero, .L_eax_010
    /* 3F45EC 004F35EC CD010000 */   break     0, 7
.align 2
.L_eax_010:
    /* 3F45F0 004F35F0 12180000 */  mflo       $v1
    /* 3F45F4 004F35F4 18886200 */  mult       $s1, $v1, $v0
    /* 3F45F8 004F35F8 2390B100 */  subu       $s2, $a1, $s1
    /* 3F45FC 004F35FC 0700201A */  blez       $s1, .L_eax_011
    /* 3F4600 004F3600 940011AE */   sw        $s1, 0x94($s0)
    /* 3F4604 004F3604 94000426 */  addiu      $a0, $s0, 0x94
    /* 3F4608 004F3608 E8CB130C */  jal        decodexac__3SndPQ23Snd10MXAPACKETF
    /* 3F460C 004F360C 21987102 */   addu      $s3, $s3, $s1
    /* 3F4610 004F3610 0800028E */  lw         $v0, 0x8($s0)
    /* 3F4614 004F3614 23105100 */  subu       $v0, $v0, $s1
    /* 3F4618 004F3618 080002AE */  sw         $v0, 0x8($s0)
.align 2
.L_eax_011:
    /* 3F461C 004F361C 2000401A */  blez       $s2, .L_eax_012
    /* 3F4620 004F3620 24000226 */   addiu     $v0, $s0, 0x24
    /* 3F4624 004F3624 A400148E */  lw         $s4, 0xA4($s0)
    /* 3F4628 004F3628 940012AE */  sw         $s2, 0x94($s0)
    /* 3F462C 004F362C 94000426 */  addiu      $a0, $s0, 0x94
    /* 3F4630 004F3630 A40002AE */  sw         $v0, 0xA4($s0)
    /* 3F4634 004F3634 E8CB130C */  jal        decodexac__3SndPQ23Snd10MXAPACKETF
    /* 3F4638 004F3638 2D880000 */   daddu     $s1, $zero, $zero
    /* 3F463C 004F363C 9400048E */  lw         $a0, 0x94($s0)
    /* 3F4640 004F3640 21307202 */  addu       $a2, $s3, $s2
    /* 3F4644 004F3644 A400058E */  lw         $a1, 0xA4($s0)
    /* 3F4648 004F3648 0800028E */  lw         $v0, 0x8($s0)
    /* 3F464C 004F364C 80180400 */  sll        $v1, $a0, 2
    /* 3F4650 004F3650 2118A300 */  addu       $v1, $a1, $v1
    /* 3F4654 004F3654 23200400 */  negu       $a0, $a0
    /* 3F4658 004F3658 23105200 */  subu       $v0, $v0, $s2
    /* 3F465C 004F365C 90FFA524 */  addiu      $a1, $a1, -0x70
    /* 3F4660 004F3660 080002AE */  sw         $v0, 0x8($s0)
    /* 3F4664 004F3664 100003AE */  sw         $v1, 0x10($s0)
    /* 3F4668 004F3668 040004AE */  sw         $a0, 0x4($s0)
    /* 3F466C 004F366C 0B00401A */  blez       $s2, .L_eax_013
    /* 3F4670 004F3670 A40005AE */   sw        $a1, 0xA4($s0)
    /* 3F4674 004F3674 2D288002 */  daddu      $a1, $s4, $zero
.align 2
.L_eax_014:
    /* 3F4678 004F3678 A400038E */  lw         $v1, 0xA4($s0)
    /* 3F467C 004F367C 80101100 */  sll        $v0, $s1, 2
    /* 3F4680 004F3680 01003126 */  addiu      $s1, $s1, 0x1
    /* 3F4684 004F3684 21104300 */  addu       $v0, $v0, $v1
    /* 3F4688 004F3688 2A203202 */  slt        $a0, $s1, $s2
    /* 3F468C 004F368C 000040C4 */  lwc1       $f0, 0x0($v0)
    /* 3F4690 004F3690 0000A0E4 */  swc1       $f0, 0x0($a1)
    /* 3F4694 004F3694 F8FF8014 */  bnez       $a0, .L_eax_014
    /* 3F4698 004F3698 0400A524 */   addiu     $a1, $a1, 0x4
.align 2
.L_eax_013:
    /* 3F469C 004F369C 2D98C000 */  daddu      $s3, $a2, $zero
.align 2
.L_eax_012:
    /* 3F46A0 004F36A0 0800028E */  lw         $v0, 0x8($s0)
    /* 3F46A4 004F36A4 01004058 */  blezl      $v0, .L_eax_015
    /* 3F46A8 004F36A8 040000AE */   sw        $zero, 0x4($s0)
.align 2
.L_eax_015:
    /* 3F46AC 004F36AC 2D106002 */  daddu      $v0, $s3, $zero
.align 2
.L_eax_006:
    /* 3F46B0 004F36B0 5000B07B */  lq         $s0, 0x50($sp)
    /* 3F46B4 004F36B4 4000B17B */  lq         $s1, 0x40($sp)
    /* 3F46B8 004F36B8 3000B27B */  lq         $s2, 0x30($sp)
    /* 3F46BC 004F36BC 2000B37B */  lq         $s3, 0x20($sp)
    /* 3F46C0 004F36C0 1000B47B */  lq         $s4, 0x10($sp)
    /* 3F46C4 004F36C4 0000BFDF */  ld         $ra, 0x0($sp)
    /* 3F46C8 004F36C8 0800E003 */  jr         $ra
    /* 3F46CC 004F36CC 6000BD27 */   addiu     $sp, $sp, 0x60
endlabel Decode__Q23Snd12CEAXABLKDecfPPfi

.align 3

glabel GetState__Q23Snd12CEAXABLKDecf
    /* 3F46D0 004F36D0 9C0080C4 */  lwc1       $f0, 0x9C($a0)
    /* 3F46D4 004F36D4 E0FFBD27 */  addiu      $sp, $sp, -0x20
    /* 3F46D8 004F36D8 980081C4 */  lwc1       $f1, 0x98($a0)
    /* 3F46DC 004F36DC FFFF0224 */  addiu      $v0, $zero, -0x1
    /* 3F46E0 004F36E0 3C100200 */  dsll32     $v0, $v0, 0
    /* 3F46E4 004F36E4 24186200 */  and        $v1, $v1, $v0
    /* 3F46E8 004F36E8 0400A0E7 */  swc1       $f0, 0x4($sp)
    /* 3F46EC 004F36EC 0000A1E7 */  swc1       $f1, 0x0($sp)
    /* 3F46F0 004F36F0 FFFF053C */  lui        $a1, (0xFFFF0000 >> 16)
    /* 3F46F4 004F36F4 3E280500 */  dsrl32     $a1, $a1, 0
    /* 3F46F8 004F36F8 0700A26B */  ldl        $v0, 0x7($sp)
    /* 3F46FC 004F36FC 0000A26F */  ldr        $v0, 0x0($sp)
    /* 3F4700 004F3700 1700A2B3 */  sdl        $v0, 0x17($sp)
    /* 3F4704 004F3704 1000A2B7 */  sdr        $v0, 0x10($sp)
    /* 3F4708 004F3708 1000A49F */  lwu        $a0, 0x10($sp)
    /* 3F470C 004F370C 1400A29F */  lwu        $v0, 0x14($sp)
    /* 3F4710 004F3710 25186400 */  or         $v1, $v1, $a0
    /* 3F4714 004F3714 24186500 */  and        $v1, $v1, $a1
    /* 3F4718 004F3718 3C100200 */  dsll32     $v0, $v0, 0
    /* 3F471C 004F371C 25106200 */  or         $v0, $v1, $v0
    /* 3F4720 004F3720 0800E003 */  jr         $ra
    /* 3F4724 004F3724 2000BD27 */   addiu     $sp, $sp, 0x20
endlabel GetState__Q23Snd12CEAXABLKDecf

.align 3

glabel SetState__Q23Snd12CEAXABLKDecfPQ23Snd8XAFSTATE
    /* 3F4728 004F3728 0000A0C4 */  lwc1       $f0, 0x0($a1)
    /* 3F472C 004F372C 980080E4 */  swc1       $f0, 0x98($a0)
    /* 3F4730 004F3730 0400A1C4 */  lwc1       $f1, 0x4($a1)
    /* 3F4734 004F3734 0800E003 */  jr         $ra
    /* 3F4738 004F3738 9C0081E4 */   swc1      $f1, 0x9C($a0)
endlabel SetState__Q23Snd12CEAXABLKDecfPQ23Snd8XAFSTATE
    /* 3F473C 004F373C 00000000 */  nop

.section .data, "wa"



dlabel _3Snd$xafilterf
    /* 54ED28 0064DD28 00000000 */ .word 0x00000000
    /* 54ED2C 0064DD2C 0000703F */ .word 0x3F700000
    /* 54ED30 0064DD30 0000E63F */ .word 0x3FE60000
    /* 54ED34 0064DD34 0000C43F */ .word 0x3FC40000
    /* 54ED38 0064DD38 00000000 */ .word 0x00000000
    /* 54ED3C 0064DD3C 00000000 */ .word 0x00000000
    /* 54ED40 0064DD40 000050BF */ .word 0xBF500000
    /* 54ED44 0064DD44 00005CBF */ .word 0xBF5C0000
enddlabel _3Snd$xafilterf

# xatablef: 16x16 de snd 9.06.00 (eaxadecf.cpp), la que enlaza hoy PS2.
dlabel _3Snd$xatablef
    /* xatablef[0] 0.0 */ .word 0x00000000
    /* xatablef[1] 4096.0 */ .word 0x45800000
    /* xatablef[2] 8192.0 */ .word 0x46000000
    /* xatablef[3] 12288.0 */ .word 0x46400000
    /* xatablef[4] 16384.0 */ .word 0x46800000
    /* xatablef[5] 20480.0 */ .word 0x46A00000
    /* xatablef[6] 24576.0 */ .word 0x46C00000
    /* xatablef[7] 28672.0 */ .word 0x46E00000
    /* xatablef[8] -32768.0 */ .word 0xC7000000
    /* xatablef[9] -28672.0 */ .word 0xC6E00000
    /* xatablef[10] -24576.0 */ .word 0xC6C00000
    /* xatablef[11] -20480.0 */ .word 0xC6A00000
    /* xatablef[12] -16384.0 */ .word 0xC6800000
    /* xatablef[13] -12288.0 */ .word 0xC6400000
    /* xatablef[14] -8192.0 */ .word 0xC6000000
    /* xatablef[15] -4096.0 */ .word 0xC5800000
    /* xatablef[16] 0.0 */ .word 0x00000000
    /* xatablef[17] 2048.0 */ .word 0x45000000
    /* xatablef[18] 4096.0 */ .word 0x45800000
    /* xatablef[19] 6144.0 */ .word 0x45C00000
    /* xatablef[20] 8192.0 */ .word 0x46000000
    /* xatablef[21] 10240.0 */ .word 0x46200000
    /* xatablef[22] 12288.0 */ .word 0x46400000
    /* xatablef[23] 14336.0 */ .word 0x46600000
    /* xatablef[24] -16384.0 */ .word 0xC6800000
    /* xatablef[25] -14336.0 */ .word 0xC6600000
    /* xatablef[26] -12288.0 */ .word 0xC6400000
    /* xatablef[27] -10240.0 */ .word 0xC6200000
    /* xatablef[28] -8192.0 */ .word 0xC6000000
    /* xatablef[29] -6144.0 */ .word 0xC5C00000
    /* xatablef[30] -4096.0 */ .word 0xC5800000
    /* xatablef[31] -2048.0 */ .word 0xC5000000
    /* xatablef[32] 0.0 */ .word 0x00000000
    /* xatablef[33] 1024.0 */ .word 0x44800000
    /* xatablef[34] 2048.0 */ .word 0x45000000
    /* xatablef[35] 3072.0 */ .word 0x45400000
    /* xatablef[36] 4096.0 */ .word 0x45800000
    /* xatablef[37] 5120.0 */ .word 0x45A00000
    /* xatablef[38] 6144.0 */ .word 0x45C00000
    /* xatablef[39] 7168.0 */ .word 0x45E00000
    /* xatablef[40] -8192.0 */ .word 0xC6000000
    /* xatablef[41] -7168.0 */ .word 0xC5E00000
    /* xatablef[42] -6144.0 */ .word 0xC5C00000
    /* xatablef[43] -5120.0 */ .word 0xC5A00000
    /* xatablef[44] -4096.0 */ .word 0xC5800000
    /* xatablef[45] -3072.0 */ .word 0xC5400000
    /* xatablef[46] -2048.0 */ .word 0xC5000000
    /* xatablef[47] -1024.0 */ .word 0xC4800000
    /* xatablef[48] 0.0 */ .word 0x00000000
    /* xatablef[49] 512.0 */ .word 0x44000000
    /* xatablef[50] 1024.0 */ .word 0x44800000
    /* xatablef[51] 1536.0 */ .word 0x44C00000
    /* xatablef[52] 2048.0 */ .word 0x45000000
    /* xatablef[53] 2560.0 */ .word 0x45200000
    /* xatablef[54] 3072.0 */ .word 0x45400000
    /* xatablef[55] 3584.0 */ .word 0x45600000
    /* xatablef[56] -4096.0 */ .word 0xC5800000
    /* xatablef[57] -3584.0 */ .word 0xC5600000
    /* xatablef[58] -3072.0 */ .word 0xC5400000
    /* xatablef[59] -2560.0 */ .word 0xC5200000
    /* xatablef[60] -2048.0 */ .word 0xC5000000
    /* xatablef[61] -1536.0 */ .word 0xC4C00000
    /* xatablef[62] -1024.0 */ .word 0xC4800000
    /* xatablef[63] -512.0 */ .word 0xC4000000
    /* xatablef[64] 0.0 */ .word 0x00000000
    /* xatablef[65] 256.0 */ .word 0x43800000
    /* xatablef[66] 512.0 */ .word 0x44000000
    /* xatablef[67] 768.0 */ .word 0x44400000
    /* xatablef[68] 1024.0 */ .word 0x44800000
    /* xatablef[69] 1280.0 */ .word 0x44A00000
    /* xatablef[70] 1536.0 */ .word 0x44C00000
    /* xatablef[71] 1792.0 */ .word 0x44E00000
    /* xatablef[72] -2048.0 */ .word 0xC5000000
    /* xatablef[73] -1792.0 */ .word 0xC4E00000
    /* xatablef[74] -1536.0 */ .word 0xC4C00000
    /* xatablef[75] -1280.0 */ .word 0xC4A00000
    /* xatablef[76] -1024.0 */ .word 0xC4800000
    /* xatablef[77] -768.0 */ .word 0xC4400000
    /* xatablef[78] -512.0 */ .word 0xC4000000
    /* xatablef[79] -256.0 */ .word 0xC3800000
    /* xatablef[80] 0.0 */ .word 0x00000000
    /* xatablef[81] 128.0 */ .word 0x43000000
    /* xatablef[82] 256.0 */ .word 0x43800000
    /* xatablef[83] 384.0 */ .word 0x43C00000
    /* xatablef[84] 512.0 */ .word 0x44000000
    /* xatablef[85] 640.0 */ .word 0x44200000
    /* xatablef[86] 768.0 */ .word 0x44400000
    /* xatablef[87] 896.0 */ .word 0x44600000
    /* xatablef[88] -1024.0 */ .word 0xC4800000
    /* xatablef[89] -896.0 */ .word 0xC4600000
    /* xatablef[90] -768.0 */ .word 0xC4400000
    /* xatablef[91] -640.0 */ .word 0xC4200000
    /* xatablef[92] -512.0 */ .word 0xC4000000
    /* xatablef[93] -384.0 */ .word 0xC3C00000
    /* xatablef[94] -256.0 */ .word 0xC3800000
    /* xatablef[95] -128.0 */ .word 0xC3000000
    /* xatablef[96] 0.0 */ .word 0x00000000
    /* xatablef[97] 64.0 */ .word 0x42800000
    /* xatablef[98] 128.0 */ .word 0x43000000
    /* xatablef[99] 192.0 */ .word 0x43400000
    /* xatablef[100] 256.0 */ .word 0x43800000
    /* xatablef[101] 320.0 */ .word 0x43A00000
    /* xatablef[102] 384.0 */ .word 0x43C00000
    /* xatablef[103] 448.0 */ .word 0x43E00000
    /* xatablef[104] -512.0 */ .word 0xC4000000
    /* xatablef[105] -448.0 */ .word 0xC3E00000
    /* xatablef[106] -384.0 */ .word 0xC3C00000
    /* xatablef[107] -320.0 */ .word 0xC3A00000
    /* xatablef[108] -256.0 */ .word 0xC3800000
    /* xatablef[109] -192.0 */ .word 0xC3400000
    /* xatablef[110] -128.0 */ .word 0xC3000000
    /* xatablef[111] -64.0 */ .word 0xC2800000
    /* xatablef[112] 0.0 */ .word 0x00000000
    /* xatablef[113] 32.0 */ .word 0x42000000
    /* xatablef[114] 64.0 */ .word 0x42800000
    /* xatablef[115] 96.0 */ .word 0x42C00000
    /* xatablef[116] 128.0 */ .word 0x43000000
    /* xatablef[117] 160.0 */ .word 0x43200000
    /* xatablef[118] 192.0 */ .word 0x43400000
    /* xatablef[119] 224.0 */ .word 0x43600000
    /* xatablef[120] -256.0 */ .word 0xC3800000
    /* xatablef[121] -224.0 */ .word 0xC3600000
    /* xatablef[122] -192.0 */ .word 0xC3400000
    /* xatablef[123] -160.0 */ .word 0xC3200000
    /* xatablef[124] -128.0 */ .word 0xC3000000
    /* xatablef[125] -96.0 */ .word 0xC2C00000
    /* xatablef[126] -64.0 */ .word 0xC2800000
    /* xatablef[127] -32.0 */ .word 0xC2000000
    /* xatablef[128] 0.0 */ .word 0x00000000
    /* xatablef[129] 16.0 */ .word 0x41800000
    /* xatablef[130] 32.0 */ .word 0x42000000
    /* xatablef[131] 48.0 */ .word 0x42400000
    /* xatablef[132] 64.0 */ .word 0x42800000
    /* xatablef[133] 80.0 */ .word 0x42A00000
    /* xatablef[134] 96.0 */ .word 0x42C00000
    /* xatablef[135] 112.0 */ .word 0x42E00000
    /* xatablef[136] -128.0 */ .word 0xC3000000
    /* xatablef[137] -112.0 */ .word 0xC2E00000
    /* xatablef[138] -96.0 */ .word 0xC2C00000
    /* xatablef[139] -80.0 */ .word 0xC2A00000
    /* xatablef[140] -64.0 */ .word 0xC2800000
    /* xatablef[141] -48.0 */ .word 0xC2400000
    /* xatablef[142] -32.0 */ .word 0xC2000000
    /* xatablef[143] -16.0 */ .word 0xC1800000
    /* xatablef[144] 0.0 */ .word 0x00000000
    /* xatablef[145] 8.0 */ .word 0x41000000
    /* xatablef[146] 16.0 */ .word 0x41800000
    /* xatablef[147] 24.0 */ .word 0x41C00000
    /* xatablef[148] 32.0 */ .word 0x42000000
    /* xatablef[149] 40.0 */ .word 0x42200000
    /* xatablef[150] 48.0 */ .word 0x42400000
    /* xatablef[151] 56.0 */ .word 0x42600000
    /* xatablef[152] -64.0 */ .word 0xC2800000
    /* xatablef[153] -56.0 */ .word 0xC2600000
    /* xatablef[154] -48.0 */ .word 0xC2400000
    /* xatablef[155] -40.0 */ .word 0xC2200000
    /* xatablef[156] -32.0 */ .word 0xC2000000
    /* xatablef[157] -24.0 */ .word 0xC1C00000
    /* xatablef[158] -16.0 */ .word 0xC1800000
    /* xatablef[159] -8.0 */ .word 0xC1000000
    /* xatablef[160] 0.0 */ .word 0x00000000
    /* xatablef[161] 4.0 */ .word 0x40800000
    /* xatablef[162] 8.0 */ .word 0x41000000
    /* xatablef[163] 12.0 */ .word 0x41400000
    /* xatablef[164] 16.0 */ .word 0x41800000
    /* xatablef[165] 20.0 */ .word 0x41A00000
    /* xatablef[166] 24.0 */ .word 0x41C00000
    /* xatablef[167] 28.0 */ .word 0x41E00000
    /* xatablef[168] -32.0 */ .word 0xC2000000
    /* xatablef[169] -28.0 */ .word 0xC1E00000
    /* xatablef[170] -24.0 */ .word 0xC1C00000
    /* xatablef[171] -20.0 */ .word 0xC1A00000
    /* xatablef[172] -16.0 */ .word 0xC1800000
    /* xatablef[173] -12.0 */ .word 0xC1400000
    /* xatablef[174] -8.0 */ .word 0xC1000000
    /* xatablef[175] -4.0 */ .word 0xC0800000
    /* xatablef[176] 0.0 */ .word 0x00000000
    /* xatablef[177] 2.0 */ .word 0x40000000
    /* xatablef[178] 4.0 */ .word 0x40800000
    /* xatablef[179] 6.0 */ .word 0x40C00000
    /* xatablef[180] 8.0 */ .word 0x41000000
    /* xatablef[181] 10.0 */ .word 0x41200000
    /* xatablef[182] 12.0 */ .word 0x41400000
    /* xatablef[183] 14.0 */ .word 0x41600000
    /* xatablef[184] -16.0 */ .word 0xC1800000
    /* xatablef[185] -14.0 */ .word 0xC1600000
    /* xatablef[186] -12.0 */ .word 0xC1400000
    /* xatablef[187] -10.0 */ .word 0xC1200000
    /* xatablef[188] -8.0 */ .word 0xC1000000
    /* xatablef[189] -6.0 */ .word 0xC0C00000
    /* xatablef[190] -4.0 */ .word 0xC0800000
    /* xatablef[191] -2.0 */ .word 0xC0000000
    /* xatablef[192] 0.0 */ .word 0x00000000
    /* xatablef[193] 1.0 */ .word 0x3F800000
    /* xatablef[194] 2.0 */ .word 0x40000000
    /* xatablef[195] 3.0 */ .word 0x40400000
    /* xatablef[196] 4.0 */ .word 0x40800000
    /* xatablef[197] 5.0 */ .word 0x40A00000
    /* xatablef[198] 6.0 */ .word 0x40C00000
    /* xatablef[199] 7.0 */ .word 0x40E00000
    /* xatablef[200] -8.0 */ .word 0xC1000000
    /* xatablef[201] -7.0 */ .word 0xC0E00000
    /* xatablef[202] -6.0 */ .word 0xC0C00000
    /* xatablef[203] -5.0 */ .word 0xC0A00000
    /* xatablef[204] -4.0 */ .word 0xC0800000
    /* xatablef[205] -3.0 */ .word 0xC0400000
    /* xatablef[206] -2.0 */ .word 0xC0000000
    /* xatablef[207] -1.0 */ .word 0xBF800000
    /* xatablef[208] 0.0 */ .word 0x00000000
    /* xatablef[209] 0.0 */ .word 0x00000000
    /* xatablef[210] 0.0 */ .word 0x00000000
    /* xatablef[211] 0.0 */ .word 0x00000000
    /* xatablef[212] 0.0 */ .word 0x00000000
    /* xatablef[213] 0.0 */ .word 0x00000000
    /* xatablef[214] 0.0 */ .word 0x00000000
    /* xatablef[215] 0.0 */ .word 0x00000000
    /* xatablef[216] 0.0 */ .word 0x00000000
    /* xatablef[217] 0.0 */ .word 0x00000000
    /* xatablef[218] 0.0 */ .word 0x00000000
    /* xatablef[219] 0.0 */ .word 0x00000000
    /* xatablef[220] 0.0 */ .word 0x00000000
    /* xatablef[221] 0.0 */ .word 0x00000000
    /* xatablef[222] 0.0 */ .word 0x00000000
    /* xatablef[223] 0.0 */ .word 0x00000000
    /* xatablef[224] 0.0 */ .word 0x00000000
    /* xatablef[225] 0.0 */ .word 0x00000000
    /* xatablef[226] 0.0 */ .word 0x00000000
    /* xatablef[227] 0.0 */ .word 0x00000000
    /* xatablef[228] 0.0 */ .word 0x00000000
    /* xatablef[229] 0.0 */ .word 0x00000000
    /* xatablef[230] 0.0 */ .word 0x00000000
    /* xatablef[231] 0.0 */ .word 0x00000000
    /* xatablef[232] 0.0 */ .word 0x00000000
    /* xatablef[233] 0.0 */ .word 0x00000000
    /* xatablef[234] 0.0 */ .word 0x00000000
    /* xatablef[235] 0.0 */ .word 0x00000000
    /* xatablef[236] 0.0 */ .word 0x00000000
    /* xatablef[237] 0.0 */ .word 0x00000000
    /* xatablef[238] 0.0 */ .word 0x00000000
    /* xatablef[239] 0.0 */ .word 0x00000000
    /* xatablef[240] 0.0 */ .word 0x00000000
    /* xatablef[241] 0.0 */ .word 0x00000000
    /* xatablef[242] 0.0 */ .word 0x00000000
    /* xatablef[243] 0.0 */ .word 0x00000000
    /* xatablef[244] 0.0 */ .word 0x00000000
    /* xatablef[245] 0.0 */ .word 0x00000000
    /* xatablef[246] 0.0 */ .word 0x00000000
    /* xatablef[247] 0.0 */ .word 0x00000000
    /* xatablef[248] 0.0 */ .word 0x00000000
    /* xatablef[249] 0.0 */ .word 0x00000000
    /* xatablef[250] 0.0 */ .word 0x00000000
    /* xatablef[251] 0.0 */ .word 0x00000000
    /* xatablef[252] 0.0 */ .word 0x00000000
    /* xatablef[253] 0.0 */ .word 0x00000000
    /* xatablef[254] 0.0 */ .word 0x00000000
    /* xatablef[255] 0.0 */ .word 0x00000000
enddlabel _3Snd$xatablef

