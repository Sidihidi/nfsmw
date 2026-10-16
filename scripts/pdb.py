#!/usr/bin/env python3
"""pdb.py FICHERO.pdb [resumen|publicos|modulos|json SALIDA] -- lector minimo de PDB de Microsoft (MSF 7.00), sin
dependencias: flujos, cabecera DBI, secciones, modulos, simbolos publicos (S_PUB32) y un resumen de los tipos (TPI).

Sirve para traer nombres de PC de un PDB (por ejemplo uno generado desde una base de IDA o desde un symbols.json):
    python scripts/pdb.py SPEED.pdb resumen
    python scripts/pdb.py SPEED.pdb json pdb_nombres.json      # {"functions": [{"start", "name"}...]}
    python scripts/nombres_pc_upstream.py pdb_nombres.json      # decide por lo que DEFINEN nuestros objetos
La direccion es 0x400000 + la VA de la seccion + el desplazamiento del simbolo. Ojo: un PDB generado puede traer los
datos de alguna seccion corridos (el de 2026-09-23 tenia .rdata corrida 0x1C); los nombres de funcion de .text si
cuadraban. Primera pasada (2026-09-23): +283 funciones de PC.
"""
import collections
import struct
import sys

MAGIC = b'Microsoft C/C++ MSF 7.00\r\n\x1aDS\x00\x00\x00'


class Pdb:
    def __init__(self, ruta):
        d = open(ruta, 'rb').read()
        assert d[:32] == MAGIC, 'no es MSF 7.00'
        self.d = d
        (self.bs, self.fpm, self.nblocks, self.ndirbytes, _, self.bmaddr) = struct.unpack_from('<6I', d, 32)
        ndirblocks = (self.ndirbytes + self.bs - 1) // self.bs
        dirblocks = struct.unpack_from('<%dI' % ndirblocks, d, self.bmaddr * self.bs)
        dirdata = b''.join(d[b * self.bs:(b + 1) * self.bs] for b in dirblocks)[:self.ndirbytes]
        nstreams, = struct.unpack_from('<I', dirdata, 0)
        sizes = struct.unpack_from('<%dI' % nstreams, dirdata, 4)
        off = 4 + 4 * nstreams
        self.streams = []
        for sz in sizes:
            if sz == 0xFFFFFFFF:
                self.streams.append((0, []))
                continue
            nb = (sz + self.bs - 1) // self.bs
            blocks = struct.unpack_from('<%dI' % nb, dirdata, off)
            off += 4 * nb
            self.streams.append((sz, blocks))

    def stream(self, i):
        sz, blocks = self.streams[i]
        return b''.join(self.d[b * self.bs:(b + 1) * self.bs] for b in blocks)[:sz]

    def dbi(self):
        s = self.stream(3)
        h = struct.unpack_from('<iIIHHHHHHiiiiiIiiHHI', s, 0)
        names = ['VersionSignature', 'VersionHeader', 'Age', 'GlobalStreamIndex', 'BuildNumber', 'PublicStreamIndex',
                 'PdbDllVersion', 'SymRecordStream', 'PdbDllRbld', 'ModInfoSize', 'SectionContributionSize',
                 'SectionMapSize', 'SourceInfoSize', 'TypeServerMapSize', 'MFCTypeServerIndex',
                 'OptionalDbgHeaderSize', 'ECSubstreamSize', 'Flags', 'Machine', 'Padding']
        return dict(zip(names, h)), s

    def modulos(self):
        h, s = self.dbi()
        off, fin = 64, 64 + h['ModInfoSize']
        out = []
        while off < fin:
            # ModInfo: Unused1 u32, SectionContr (28 B), Flags u16, ModuleSymStream u16, SymByteSize u32,
            # C11ByteSize u32, C13ByteSize u32, SourceFileCount u16, Padding u16, Unused2 u32, SourceFileNameIndex u32,
            # PdbFilePathNameIndex u32, ModuleName (cstr), ObjFileName (cstr), alineado a 4
            flags, symstream, symbytes, c11, c13, nsrc = struct.unpack_from('<HHIIIH', s, off + 4 + 28)
            p = off + 4 + 28 + 2 + 2 + 4 + 4 + 4 + 2 + 2 + 4 + 4 + 4
            e1 = s.index(b'\0', p)
            nombre = s[p:e1].decode('latin-1')
            e2 = s.index(b'\0', e1 + 1)
            obj = s[e1 + 1:e2].decode('latin-1')
            off = (e2 + 1 + 3) & ~3
            out.append((nombre, obj, symstream, symbytes))
        return out

    def secciones(self):
        h, s = self.dbi()
        off = 64 + h['ModInfoSize'] + h['SectionContributionSize'] + h['SectionMapSize'] + h['SourceInfoSize'] + \
            h['TypeServerMapSize'] + h['ECSubstreamSize']
        dbg = struct.unpack_from('<%dH' % (h['OptionalDbgHeaderSize'] // 2), s, off)
        idx = dbg[5] if len(dbg) > 5 else 0xFFFF
        if idx == 0xFFFF:
            return []
        sh = self.stream(idx)
        out = []
        for k in range(len(sh) // 40):
            nombre, vsize, va = struct.unpack_from('<8sII', sh, k * 40)
            out.append((nombre.rstrip(b'\0').decode('latin-1'), va, vsize))
        return out

    def registros(self, datos, inicio=0):
        off = inicio
        while off + 4 <= len(datos):
            ln, kind = struct.unpack_from('<HH', datos, off)
            if ln < 2:
                break
            yield kind, datos[off + 4:off + 2 + ln]
            off += 2 + ln

    def simbolos_globales(self):
        h, _ = self.dbi()
        return self.registros(self.stream(h['SymRecordStream']))


def cstr(b, off):
    e = b.index(b'\0', off)
    return b[off:e].decode('latin-1')


KINDS = {0x110E: 'S_PUB32', 0x110D: 'S_GDATA32', 0x110C: 'S_LDATA32', 0x1125: 'S_PROCREF', 0x1127: 'S_LPROCREF',
         0x1108: 'S_UDT', 0x1107: 'S_CONSTANT', 0x1110: 'S_GPROC32', 0x110F: 'S_LPROC32', 0x1111: 'S_REGREL32',
         0x1006: 'S_REGISTER', 0x1012: 'S_FRAMEPROC', 0x0006: 'S_END', 0x113C: 'S_COMPILE3', 0x1101: 'S_OBJNAME',
         0x1136: 'S_SECTION', 0x1137: 'S_COFFGROUP', 0x1102: 'S_THUNK32', 0x1105: 'S_LABEL32', 0x1103: 'S_BLOCK32',
         0x1106: 'S_BPREL32'}


def main():
    p = Pdb(sys.argv[1])
    modo = sys.argv[2] if len(sys.argv) > 2 else 'resumen'
    if modo == 'resumen':
        print('bloque %d, %d bloques, %d flujos' % (p.bs, p.nblocks, len(p.streams)))
        for i, (sz, _) in enumerate(p.streams[:12]):
            print('  flujo %2d: %9d B' % (i, sz))
        h, _ = p.dbi()
        print('DBI:', {k: h[k] for k in ('VersionHeader', 'Age', 'GlobalStreamIndex', 'PublicStreamIndex',
                                          'SymRecordStream', 'ModInfoSize', 'SectionContributionSize',
                                          'OptionalDbgHeaderSize', 'Machine')})
        print('secciones:', p.secciones())
        c = collections.Counter(KINDS.get(k, hex(k)) for k, _ in p.simbolos_globales())
        print('simbolos globales:', c.most_common())
        mods = p.modulos()
        print('modulos: %d' % len(mods))
        for m in mods[:15]:
            print('   ', m)
        tpi = p.stream(2)
        ver, hsz, tmin, tmax, tbytes = struct.unpack_from('<IIIII', tpi, 0)
        c2 = collections.Counter()
        for kind, _ in p.registros(tpi, hsz):
            c2[kind] += 1
        print('TPI: tipos %d..%d (%d B); clases de registro:' % (tmin, tmax, tbytes),
              ', '.join('%#x:%d' % (k, v) for k, v in c2.most_common(12)))
    elif modo == 'publicos':
        secs = p.secciones()
        for kind, r in p.simbolos_globales():
            if kind == 0x110E:
                flags, off, seg = struct.unpack_from('<IIH', r, 0)
                nombre = cstr(r, 10)
                va = 0x400000 + secs[seg - 1][1] + off if 0 < seg <= len(secs) else None
                print('%08x %d %s' % (va or 0, flags, nombre))
    elif modo == 'modulos':
        for m in p.modulos():
            print(m)
    elif modo == 'json':
        import json
        secs = p.secciones()
        funcs = []
        for kind, r in p.simbolos_globales():
            if kind != 0x110E:
                continue
            flags, off, seg = struct.unpack_from('<IIH', r, 0)
            if not (0 < seg <= len(secs)) or secs[seg - 1][0] != '.text':
                continue
            funcs.append({'start': 0x400000 + secs[seg - 1][1] + off, 'name': cstr(r, 10)})
        json.dump({'functions': funcs}, open(sys.argv[3], 'w', encoding='utf-8'), indent=1)
        print('%d simbolos de .text escritos en %s' % (len(funcs), sys.argv[3]))


if __name__ == '__main__':
    main()
