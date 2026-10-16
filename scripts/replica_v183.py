#!/usr/bin/env python3
"""replica_v183.py [--forzar] -- genera las toolchains de la libreria `path` a partir de las oficiales:

    build/compilers/ProDG/3.9.3-v183r            desde ProDG/3.9.3            (GameCube)
    build/compilers/PS2/ee-gcc2.95.3-136-v183r   desde PS2/ee-gcc2.95.3-136   (las dos PS2)

POR QUE. El DWARF de GameCube dice que path se compilo con "GNU C++ 2.95.3 SN BUILD v1.83" (paquete de EA
C:\\packages\\GameCubeSN\\3.9.3-4-non-proxy) y el .mdebug de la A124 que su path salio de PlayStation2SN 3.0.2-4; el
resto del juego y del middleware es v1.76 / 3.0.2-1 o anterior. Esos paquetes no existen en ningun sitio publico
(decomp.me, archive.org y el disco llegan a v1.76 / build 1.36). Lo que los separa de los nuestros es UN cambio real
de GCC, publicado en el ChangeLog del 2000-06-23 (Geoffrey Keating):

    * alias.c (fixed_scalar_and_varying_struct_p): Don't examine struct vs. scalar-ness when -fno-strict-aliasing.

Entro en GCC 3.0 y no en la rama 2.95; la fuente GPL de SN v1.79 (orig/prodg/ProDGforNGCv393_Source_Code.zip) aun
aplica la regla, asi que SN lo adopto entre v1.79 y v1.83. Con -fno-strict-aliasing (el valor por defecto de 2.95 y el
unico que usa este proyecto) ese cambio equivale a que la funcion devuelva siempre NULL: este guion pone
`xor eax,eax; ret` en sus tres primeros bytes, en cc1 y cc1plus, localizandola por su cuerpo.

MEDIDO (ronda 13, fuera del arbol, flags de configure.py): path en GameCube 182 -> 193 de 194 funciones, en cada PS2
+6 y 4 mas suben, cero perdidas; y en pathserv.cpp el `*(unsigned long *)&` que esquivaba la regla sobra. Control
negativo: la misma replica en las bibliotecas v1.76 pierde 59 funciones en GameCube y 28 en cada PS2: solo vale para
path, que es justo la que el DWARF marca v1.83.

Cada fichero se comprueba por SHA-1 antes (el oficial) y despues (la replica): si algo no cuadra, no se genera nada.
"""
import hashlib
import os
import re
import shutil
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
COMPILADORES = os.path.join(RAIZ, 'build', 'compilers')

# (toolchain oficial, toolchain replica, directorio de cc1 dentro, {fichero: (sha1 oficial, sha1 replica)})
REPLICAS = [
    ('ProDG/3.9.3', 'ProDG/3.9.3-v183r', '.', {
        'cc1.exe': ('63d2bbfd0948616095ac3e4cceb0e7ced87c5101', '57bb97e8dbf14c18eecbbbc627a6d41f80085b7a'),
        'cc1plus.exe': ('9307cd3263a3e82e722ecc86691a7c20b547b5f3', '31036ffc80b41a7a0f2a2979401d3c1367fd8f91'),
    }),
    ('PS2/ee-gcc2.95.3-136', 'PS2/ee-gcc2.95.3-136-v183r', 'lib/gcc-lib/ee/2.95.3', {
        'cc1.exe': ('becaeaee0255adddb73d6f41bd0a3c8a0882eb2c', 'a21bcc82e20793adc74a3734dec9f87caef72090'),
        'cc1plus.exe': ('2e7192cd633a2a04f2a78fe84e4ac0fdbd6867ad', '7f6cf7c318b1aba463251e144ae531bf18317f7e'),
    }),
]

# El cuerpo compilado de fixed_scalar_and_varying_struct_p: XEXP (mem, 0) en +8 y las banderas en el byte 5 del rtx
# (MEM_IN_STRUCT_P / MEM_SCALAR_P), probadas dos veces con las mascaras cruzadas.
FIRMA = re.compile(rb'\x53\x8b\x5c\x24\x0c\x55\x56\x8b\x74\x24\x10\x8b\x6b\x08\x57\x8b\x46\x04\x8b\x7e\x08'
                   rb'\xf6\xc4(.)\x74.\x8b\x43\x04\xf6\xc4(.)\x74.', re.S)
DEVUELVE_NULL = b'\x33\xc0\xc3'  # xor eax,eax ; ret


def sha1(datos):
    return hashlib.sha1(datos).hexdigest()


def parchea(datos):
    ms = list(FIRMA.finditer(datos))
    if len(ms) != 1:
        sys.exit('la firma de fixed_scalar_and_varying_struct_p aparece %d veces: no se genera nada' % len(ms))
    o = ms[0].start()
    m1, m2 = ms[0].group(1)[0], ms[0].group(2)[0]
    resto = bytes(datos[o:o + 160])
    if bytes([0xf6, 0xc4, m2]) not in resto[40:] or bytes([0xf6, 0xc4, m1]) not in resto[40:]:
        sys.exit('el cuerpo no es el de fixed_scalar_and_varying_struct_p: no se genera nada')
    b = bytearray(datos)
    b[o:o + 3] = DEVUELVE_NULL
    return bytes(b)


def al_dia(dst, sub, ficheros):
    return all(os.path.isfile(os.path.join(dst, sub, f)) and
               sha1(open(os.path.join(dst, sub, f), 'rb').read()) == despues
               for f, (antes, despues) in ficheros.items())


def main():
    forzar = '--forzar' in sys.argv
    for oficial, replica, sub, ficheros in REPLICAS:
        src = os.path.join(COMPILADORES, *oficial.split('/'))
        dst = os.path.join(COMPILADORES, *replica.split('/'))
        if not forzar and al_dia(dst, sub, ficheros):
            print('%s: al dia' % replica)
            continue
        if not os.path.isdir(src):
            sys.exit('%s no existe: hace falta la toolchain oficial %s' % (src, oficial))
        # primero todo en memoria: si un SHA-1 no cuadra no se toca el disco
        nuevos = {}
        for f, (antes, despues) in ficheros.items():
            datos = open(os.path.join(src, sub, f), 'rb').read()
            if sha1(datos) != antes:
                sys.exit('%s/%s no es el oficial (sha1 %s, se esperaba %s)' % (oficial, f, sha1(datos), antes))
            nuevo = parchea(datos)
            if sha1(nuevo) != despues:
                sys.exit('la replica de %s/%s no da el sha1 esperado (%s)' % (oficial, f, sha1(nuevo)))
            nuevos[f] = nuevo
        if os.path.lexists(dst):
            if os.path.islink(dst) or (hasattr(os.path, 'isjunction') and os.path.isjunction(dst)):
                sys.exit('%s es un enlace o una union: no se borra' % dst)
            shutil.rmtree(dst)
        shutil.copytree(src, dst)
        for f, nuevo in nuevos.items():
            open(os.path.join(dst, sub, f), 'wb').write(nuevo)
        print('%s: generada desde %s (%s)' % (replica, oficial, ', '.join(sorted(nuevos))))


if __name__ == '__main__':
    main()
