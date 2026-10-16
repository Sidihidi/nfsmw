#!/usr/bin/env python3
"""clase_stabs.py <Clase> [...] -- lo que los stabs de la A124 saben de una clase: tamano, cada campo con su
offset y SU TIPO, y la lista de metodos con su firma.

Para escribir de cero las clases que solo existen en PS2 (las de `src/Speed/PSX2/`, que en nuestro arbol son
ficheros vacios). Los stabs de GCC 2.9 guardan el campo como `nombre:/0<tipo>,<offset en bits>,<tamano en bits>;`
y el metodo como `nombre::<n>=##<tipo de retorno>;:<argumentos decorados>;`, asi que de ahi salen las tres cosas
que hacen falta para declararla: el layout, los tipos y que accesores tiene (como el DWARF en GameCube).

El numero de tipo de un campo no se puede resolver leyendo el texto (no hay ninguna definicion `<n>=...`: es un
indice de la tabla auxiliar de ECOFF, comprobado buscandolo en toda la seccion). **Pero es consistente**, asi que
basta ANCLAR unos pocos con el gemelo de otra plataforma y quedan nombrados en todas las clases: los de ANCLAS,
medidos con `src/Speed/GameCube/Src/Ecstasy/EcstasyE.hpp` (ScissorX es int, Name es const char *) y con los
punteros a matriz de eViewPlatInfo. Un campo cuyo tipo no este anclado sale como `tipo<n>` y no se inventa:
buscar la definicion por texto da nombres FALSOS, porque el numero es local al fichero y otra clase lo reutiliza
(salia `CSTATEMGR_Music` para un puntero). Para nombrarlo, mira otra clase que use ese mismo numero en un campo
que ya conozcas, o el desensamblado (`lwc1` es float, `lw` no).

    python scripts/clase_stabs.py eRenderTarget
    python scripts/clase_stabs.py eViewPlatInfo eRenderTarget
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__))).replace('\\', '/') + '/'
sys.path.insert(0, ROOT + 'scripts')
import mdebug  # noqa: E402

# Un campo es `nombre:<tipo>,<offset en bits>,<tamano en bits>;`. Los de un `struct` no llevan la marca de
# visibilidad `/0` que si llevan los de una `class`, y el tipo puede venir DEFINIDO ahi mismo, con sus
# propios `;` dentro (`Views:23569=ar1;0;5;2565,0,192;` es un array de seis), asi que hay que llegar hasta
# el `,<offset>,<tamano>;` final sin cortar en el primer `;`.
CAMPO = re.compile(r'([A-Za-z_$][\w$]*):(?:/\d)?(.+?),(\d+),(\d+);')
METODO = re.compile(r'([A-Za-z_~][\w]*)::?\d*=?##(\d+);:([^;]*);')

# Anclas: numero de tipo -> tipo de C++. Los bajos son los tipos basicos, y salen de comparar el volcado de
# eSolid con la declaracion que ya tenemos en src/Speed/Indep/Src/Ecstasy/Ecstasy.hpp (Version es uint8, Flags
# uint16, NameHash uint32, NumPolys int16, NumBones int8, AABBMinX float); los altos, de eRenderTarget y
# eViewPlatInfo contra el gemelo de GameCube.
ANCLAS = {
    '14': 'float',           # eSolid::AABBMinX, eRenderTarget::AspectRatioX/FovBias/ZMax
    '26': 'signed char',     # eSolid::NumBones (int8)
    '27': 'unsigned char',   # eSolid::Version, EndianSwapped (uint8)
    '28': 'short',           # eSolid::NumPolys, NumVerts (int16)
    '29': 'unsigned short',  # eSolid::Flags (uint16)
    '30': 'int',             # eSolid::ReferencedFrameCounter (int32), eRenderTarget::ScissorX/Y/W/H
    '31': 'unsigned int',    # eSolid::NameHash (uint32)
    '225': 'const char *',   # eRenderTarget::Name (const char * en GameCube)
    '2113': 'bMatrix4',      # eSolid::PivotMatrix (64 B)
    '2115': 'bMatrix4 *',    # eViewPlatInfo::WorldViewMatrix y las demas matrices
}


def cadenas():
    with open(mdebug.ELF, 'rb') as f:
        f.seek(mdebug.SS_OFF)
        return f.read(mdebug.SS_SIZE + 0x2000000)


def stab_de(ss, nombre):
    """La cadena de stabs de la definicion de una clase, y donde empieza."""
    m = re.search(re.escape(nombre.encode()) + rb':T[t]?\d+=s(?=\d)', ss)
    if not m:
        return None, None
    fin = ss.index(b'\0', m.end())
    s = ss[m.end():fin].decode('latin-1')
    p = fin
    while s.endswith('\\'):
        q = ss.index(b'\0', p + 1)
        s = s[:-1] + ss[p + 1:q].decode('latin-1')
        p = q
    return s, m.start()


def main():
    pedidas = [a for a in sys.argv[1:] if not a.startswith('--')]
    if not pedidas:
        sys.exit(__doc__)
    ss = cadenas()
    for pedida in pedidas:
        s, pos = stab_de(ss, pedida)
        if s is None:
            print('== %s: no esta en los stabs' % pedida)
            continue
        tam = int(re.match(r'(\d+)', s).group(1))
        print('== %s: %d B (0x%x)' % (pedida, tam, tam))
        cache = {}
        # los metodos empiezan con `nombre::`, y ahi se acaban los campos
        m = re.search(r'[A-Za-z_~][\w]*::', s)
        campos = s[:m.start()] if m else s
        for nom, blob, off, bits in CAMPO.findall(campos):
            m = re.match(r'(\d+)', blob)
            tipo = m.group(1) if m else blob
            t = cache.get(blob)
            if t is None:
                t = ANCLAS.get(tipo) or ('tipo' + tipo + (' []' if '=ar' in blob else ''))
                cache[blob] = t
            print('   0x%04x  %4d B  %-28s %s' % (int(off) // 8, int(bits) // 8, t, nom))
        vistos = []
        for nom, ret, args in METODO.findall(s):
            if (nom, args) not in vistos:
                vistos.append((nom, args))
        if vistos:
            print('   -- metodos (argumentos decorados):')
            for nom, args in vistos:
                print('      %-34s (%s)' % (nom, args or 'void'))


if __name__ == '__main__':
    main()
