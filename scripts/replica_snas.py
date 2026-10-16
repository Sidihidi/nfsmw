#!/usr/bin/env python3
"""replica_snas.py [--forzar] -- genera la toolchain de la libc de SN de PS2 a partir de la oficial:

    build/compilers/PS2/ee-gcc2.95.3-136-snas   desde PS2/ee-gcc2.95.3-136   (las dos PS2)

POR QUE. La libc de SN del juego (objetos .obj del NFS.MAP de la A124: newlib 1.8.1 compilado por SN, y las funciones
de cadena en asm de newlib/libc/machine/r5900) se ensamblo con el ensamblador de SN, Ps2EeAs, y no con el as de GNU:
el `dli` de los .S sale como lo expande Ps2EeAs. Pero no con el nuestro (ps2eeas 1.9.25.758): los dos alargan los
bucles cortos (el fallo del R5900 con bucles de 6 instrucciones o menos) metiendo nop delante del salto hacia atras,
y el del original los deja en 6 instrucciones donde el nuestro los deja en 7. Todo lo demas es igual.

En el .exe lo hace la funcion de 0x404d90: con si = desplazamiento del salto en instrucciones (negativo),

    cmp si, -6 ; jle fin       solo los bucles cortos
    lea esi, [eax + 6]         cuantos nop
    ... emite esi nop

La replica cambia las dos constantes 6 por 5 (2 bytes). Ningun otro byte cambia.

MEDIDO (ronda 36, fuera del arbol): las 13 funciones r5900 casan en las dos PS2 (con el oficial, 3 de 13), y el C
de la libc de SN ensamblado con ella (ee-gcc -S y Ps2EeAs) gana 16 funciones en la A124 y 15 en SLUS (mprec,
_strtod_r, _strtol_r, _strtoul_r, exit, strstr, __sfvwrite, _malloc_r, __sfp) sin perder ninguna de sus 60 unidades.

Cada fichero se comprueba por SHA-1 antes (el oficial) y despues (la replica): si algo no cuadra, no se genera nada.
El .exe oficial no se toca.
"""
import hashlib
import os
import shutil
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
COMPILADORES = os.path.join(RAIZ, 'build', 'compilers')

OFICIAL = 'PS2/ee-gcc2.95.3-136'
REPLICA = 'PS2/ee-gcc2.95.3-136-snas'
FICHERO = 'ee/bin/Ps2EeAs.exe'
SHA_OFICIAL = 'b49b032e8fe028f4c98fff0de7282ab93e295147'
SHA_REPLICA = 'a620bfcb50b5c330b5c498d8109265b0ddfa567e'
# (desplazamiento en el fichero, bytes del oficial, bytes de la replica)
PARCHES = [
    (0x4db6, bytes.fromhex('6683fefa'), bytes.fromhex('6683fefb')),  # cmp si, -6       ->  cmp si, -5
    (0x4de9, bytes.fromhex('8d7006'), bytes.fromhex('8d7005')),  # lea esi, [eax+6]  ->  lea esi, [eax+5]
]


def sha1(datos):
    return hashlib.sha1(datos).hexdigest()


def main():
    forzar = '--forzar' in sys.argv
    src = os.path.join(COMPILADORES, *OFICIAL.split('/'))
    dst = os.path.join(COMPILADORES, *REPLICA.split('/'))
    destino = os.path.join(dst, *FICHERO.split('/'))
    if not forzar and os.path.isfile(destino) and sha1(open(destino, 'rb').read()) == SHA_REPLICA:
        print('%s: al dia' % REPLICA)
        return
    if not os.path.isdir(src):
        sys.exit('%s no existe: hace falta la toolchain oficial %s' % (src, OFICIAL))
    # primero en memoria: si un SHA-1 no cuadra no se toca el disco
    datos = open(os.path.join(src, *FICHERO.split('/')), 'rb').read()
    if sha1(datos) != SHA_OFICIAL:
        sys.exit('%s/%s no es el oficial (sha1 %s, se esperaba %s)' % (OFICIAL, FICHERO, sha1(datos), SHA_OFICIAL))
    b = bytearray(datos)
    for off, antes, despues in PARCHES:
        if bytes(b[off:off + len(antes)]) != antes:
            sys.exit('%s/%s: bytes inesperados en 0x%x: no se genera nada' % (OFICIAL, FICHERO, off))
        b[off:off + len(antes)] = despues
    if sha1(b) != SHA_REPLICA:
        sys.exit('la replica de %s/%s no da el sha1 esperado (%s)' % (OFICIAL, FICHERO, sha1(b)))
    if os.path.lexists(dst):
        if os.path.islink(dst) or (hasattr(os.path, 'isjunction') and os.path.isjunction(dst)):
            sys.exit('%s es un enlace o una union: no se borra' % dst)
        shutil.rmtree(dst)
    shutil.copytree(src, dst)
    open(destino, 'wb').write(bytes(b))
    print('%s: generada desde %s (%s)' % (REPLICA, OFICIAL, FICHERO))


if __name__ == '__main__':
    main()
