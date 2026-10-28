#!/usr/bin/env python3
"""xexdec.py ENTRADA.xex SALIDA.exe -- saca la imagen PE (en memoria: secciones por direccion virtual) de un XEX2 de
Xbox 360 de DESARROLLO (clave de devkit: 16 ceros) comprimido con LZX ("normal") o sin comprimir.

SOLO CLAVES DE DESARROLLO. No hay opcion para dar otra clave: si el XEX no se descifra con la de devkit (un XEX de
VENTA, firmado con la clave de la consola) se para sin escribir nada. Los XEX de venta no se tocan (decision del
usuario, ronda 78).

Procedencia: carril PROTOS-MW de la ronda 78 (build/scratch_protosmw/, docs/analisis/2026-10-21_protosmw.md), pasado a
scripts/ en la ronda 79. Validado: el NfsMWEuropeGerMilestone.xex (LZX) del prototipo del 21-oct da una imagen cuyas
16 secciones con contenido son iguales a las de orig/EUROPEGERMILESTONE/NFS.exe (scripts/pecmp.py; la .reloc no, la
imagen no la trae), y el DEFAULT.XEX del prototipo XENON 07.42 da 13.434.880 B, timestamp 1129880560.

    python scripts/xexdec.py DEFAULT.XEX xenon.exe
    python scripts/pecmp.py xenon.exe orig/EUROPEGERMILESTONE/NFS.exe

La imagen se lee con scripts/ppcsk.py (base de ejecucion 0x82000000) tomando las secciones por direccion virtual.

Pasos (formato de Xenia, xex_module.cc):
  1. cabeceras XEX2 y la cabecera opcional 0x3FF (cifrado y compresion);
  2. clave de sesion = AES-ECB(clave de devkit) de security_info+0x150; cuerpo desde el desplazamiento de datos con
     AES-CBC (IV a cero);
  3. cadena de bloques: cada bloque empieza con el tamano y el SHA-1 del SIGUIENTE (el primero viene en la cabecera:
     se comprueban TODOS los hashes); dentro, trozos LZX con longitud u16 BE hasta un 0;
  4. LZX (el de los CAB / XMemCompress, bloques con tamano de 24 bits, tramas de 32 KB realineadas a 16 bits) con la
     ventana de la cabecera, hasta image_size bytes.
Solo para ejecutables de desarrollo: con otra clave se para (los de venta no se tocan).
Sin dependencias salvo pycryptodomex (Cryptodome) o pycryptodome (Crypto).
"""
import hashlib
import struct
import sys

try:
    from Cryptodome.Cipher import AES
except ImportError:
    from Crypto.Cipher import AES

DEVKIT = bytes(16)


def be32(d, o):
    return struct.unpack_from('>I', d, o)[0]


def be16(d, o):
    return struct.unpack_from('>H', d, o)[0]


def cabeceras(d):
    if d[:4] != b'XEX2':
        sys.exit('no es un XEX2')
    datos = be32(d, 8)
    seguridad = be32(d, 0x10)
    n = be32(d, 0x14)
    opt = {}
    for i in range(n):
        k, v = struct.unpack_from('>II', d, 0x18 + 8 * i)
        opt[k] = v
    return datos, seguridad, opt


# ---------------------------------------------------------------- LZX (puerto de la logica de libmspack lzxd.c)
EXTRA = [0, 0, 0, 0] + [i // 2 for i in range(2, 34)]
EXTRA = [min(e, 17) for e in EXTRA] + [17] * 20
BASE = []
_b = 0
for _e in EXTRA:
    BASE.append(_b)
    _b += 1 << _e
SLOTS = {15: 30, 16: 32, 17: 34, 18: 36, 19: 38, 20: 42, 21: 50}


class Bits:
    def __init__(self, data):
        if len(data) & 1:
            data += b'\0'
        self.w = struct.unpack('<%dH' % (len(data) // 2), data)
        self.i = 0
        self.buf = 0
        self.n = 0

    def ensure(self, k):
        while self.n < k:
            w = self.w[self.i] if self.i < len(self.w) else 0
            self.i += 1
            self.buf = ((self.buf << 16) | w)
            self.n += 16

    def peek(self, k):
        self.ensure(k)
        return (self.buf >> (self.n - k)) & ((1 << k) - 1)

    def drop(self, k):
        self.n -= k
        self.buf &= (1 << self.n) - 1

    def read(self, k):
        if k == 0:
            return 0
        v = self.peek(k)
        self.drop(k)
        return v

    def align16(self):
        # libmspack: al final de cada trama de 32 KB, ENSURE_BITS(16) y quitar bits_left & 15
        if self.n > 0:
            self.ensure(16)
        if self.n & 15:
            self.drop(self.n & 15)


def tabla(longitudes):
    """Tabla de decodificacion canonica: (bits maximos, lista de (simbolo, longitud) indexada por el prefijo)."""
    mx = max(longitudes) if any(longitudes) else 1
    t = [None] * (1 << mx)
    code = 0
    for ln in range(1, mx + 1):
        for s, l in enumerate(longitudes):
            if l == ln:
                rep = 1 << (mx - ln)
                base = code << (mx - ln)
                if base + rep > len(t):
                    raise ValueError('arbol de Huffman sobresuscrito')
                for j in range(base, base + rep):
                    t[j] = (s, ln)
                code += 1
        code <<= 1
    return mx, t


def simbolo(bits, tb):
    mx, t = tb
    e = t[bits.peek(mx)]
    if e is None:
        raise ValueError('codigo de Huffman invalido')
    bits.drop(e[1])
    return e[0]


def lee_longitudes(bits, lens, a, b):
    pre = [bits.read(4) for _ in range(20)]
    pt = tabla(pre)
    x = a
    while x < b:
        z = simbolo(bits, pt)
        if z == 17:
            for _ in range(bits.read(4) + 4):
                lens[x] = 0
                x += 1
        elif z == 18:
            for _ in range(bits.read(5) + 20):
                lens[x] = 0
                x += 1
        elif z == 19:
            n = bits.read(1) + 4
            z = simbolo(bits, pt)
            v = (lens[x] - z + 17) % 17
            for _ in range(n):
                lens[x] = v
                x += 1
        else:
            lens[x] = (lens[x] - z + 17) % 17
            x += 1


def lzx(data, ventana, total):
    wb = ventana.bit_length() - 1
    if (1 << wb) != ventana or wb not in SLOTS:
        sys.exit('ventana LZX no valida: %#x' % ventana)
    nmain = 256 + SLOTS[wb] * 8
    main_l = [0] * nmain
    len_l = [0] * 249
    bits = Bits(data)
    out = bytearray()
    r0 = r1 = r2 = 1
    intel = bits.read(1)
    if intel:
        bits.read(32)
        print('aviso: traduccion E8 marcada (no se deshace; en codigo PPC no deberia aparecer)')
    resto = 0
    tipo = 0
    main_t = len_t = ali_t = None
    trama_fin = 0x8000
    while len(out) < total:
        if resto == 0:
            tipo = bits.read(3)
            tam_bloque = resto = (bits.read(16) << 8) | bits.read(8)
            if tipo == 2:
                ali_t = tabla([bits.read(3) for _ in range(8)])
            if tipo in (1, 2):
                lee_longitudes(bits, main_l, 0, 256)
                lee_longitudes(bits, main_l, 256, nmain)
                main_t = tabla(main_l)
                lee_longitudes(bits, len_l, 0, 249)
                len_t = tabla(len_l) if any(len_l) else None
            elif tipo == 3:
                # realinear a 16 bits y leer R0-R2 en crudo
                bits.ensure(16)
                if bits.n > 16:
                    bits.i -= 1
                bits.buf = 0
                bits.n = 0
                raw = b''.join(struct.pack('<H', bits.w[bits.i + k]) for k in range(6))
                bits.i += 6
                r0, r1, r2 = struct.unpack('<III', raw)
            else:
                sys.exit('bloque LZX de tipo %d en %d' % (tipo, len(out)))
        # decodificar hasta el final del bloque o de la trama
        hasta = min(len(out) + resto, trama_fin, total)
        antes = len(out)
        if tipo == 3:
            n = hasta - len(out)
            # bytes crudos desde la posicion de palabra actual
            off = bits.i * 2
            crudo = b''.join(struct.pack('<H', w) for w in bits.w[bits.i:bits.i + (n + 2) // 2 + 1])
            out += crudo[:n]
            consumidos = n
            if resto - n == 0 and (tam_bloque & 1):
                consumidos += 1  # relleno
            bits.i += consumidos // 2
            if consumidos & 1:
                # medio par de bytes: reconstruir el estado (raro); se rehace el lector desde el byte
                byte_pos = off + consumidos
                rest = bytes(b for w in bits.w for b in struct.pack('<H', w))[byte_pos:]
                bits = Bits(rest)
            bits.buf = 0
            bits.n = 0
        else:
            while len(out) < hasta:
                s = simbolo(bits, main_t)
                if s < 256:
                    out.append(s)
                    continue
                s -= 256
                ml = s & 7
                if ml == 7:
                    ml += simbolo(bits, len_t)
                ml += 2
                slot = s >> 3
                if slot == 0:
                    off = r0
                elif slot == 1:
                    off = r1
                    r1 = r0
                    r0 = off
                elif slot == 2:
                    off = r2
                    r2 = r0
                    r0 = off
                else:
                    ex = EXTRA[slot]
                    off = BASE[slot] - 2
                    if tipo == 2:
                        if ex > 3:
                            off += bits.read(ex - 3) << 3
                            off += simbolo(bits, ali_t)
                        elif ex == 3:
                            off += simbolo(bits, ali_t)
                        elif ex > 0:
                            off += bits.read(ex)
                        else:
                            off = 1
                    else:
                        off += bits.read(ex)
                    r2 = r1
                    r1 = r0
                    r0 = off
                if off > len(out):
                    raise ValueError('desplazamiento %d fuera de la salida (%d)' % (off, len(out)))
                ini = len(out) - off
                if off >= ml:
                    out += out[ini:ini + ml]
                else:
                    for k in range(ml):
                        out.append(out[ini + k])
        resto -= len(out) - antes
        hecho = len(out)
        if hecho >= trama_fin:
            trama_fin += 0x8000
            if tipo != 3:
                bits.align16()
    return bytes(out[:total])


def main():
    if len(sys.argv) != 3 or any(a in ('-h', '--help') or a.startswith('--') for a in sys.argv[1:]):
        sys.exit(__doc__)
    d = open(sys.argv[1], 'rb').read()
    datos, seg, opt = cabeceras(d)
    image_size = be32(d, seg + 4)
    if 0x3FF not in opt:
        sys.exit('sin cabecera de formato (0x3FF)')
    ff = opt[0x3FF]
    cifrado = be16(d, ff + 4)
    compresion = be16(d, ff + 6)
    print('imagen %#x, datos desde %#x, cifrado %d, compresion %d' % (image_size, datos, cifrado, compresion))
    cuerpo = d[datos:]
    if cifrado == 1:
        sesion = AES.new(DEVKIT, AES.MODE_ECB).decrypt(d[seg + 0x150:seg + 0x160])
        n = len(cuerpo) & ~15
        cuerpo = AES.new(sesion, AES.MODE_CBC, bytes(16)).decrypt(cuerpo[:n]) + cuerpo[n:]
    if compresion == 1:
        sys.exit('compresion basica (por bloques con ceros): no implementada')
    if compresion == 0:
        img = cuerpo[:image_size]
    elif compresion == 2:
        ventana = be32(d, ff + 8)
        tam = be32(d, ff + 12)
        h = d[ff + 16:ff + 36]
        p = 0
        trozos = []
        nb = 0
        while tam:
            bloque = cuerpo[p:p + tam]
            if hashlib.sha1(bloque).digest() != h:
                sys.exit('el SHA-1 del bloque %d no cuadra: no es un XEX de desarrollo (o el formato no es el '
                         'esperado). Solo se descifran XEX de devkit: se para sin escribir nada' % nb)
            sig = be32(bloque, 0)
            h = bloque[4:24]
            q = 24
            while True:
                c = be16(bloque, q)
                q += 2
                if c == 0:
                    break
                trozos.append(bloque[q:q + c])
                q += c
            p += tam
            tam = sig
            nb += 1
        lz = b''.join(trozos)
        print('%d bloques verificados por SHA-1, %d bytes LZX, ventana %#x' % (nb, len(lz), ventana))
        img = lzx(lz, ventana, image_size)
    else:
        sys.exit('compresion %d desconocida' % compresion)
    if img[:2] != b'MZ':
        sys.exit('la imagen no empieza por MZ: no es un XEX de desarrollo (clave de devkit); no se escribe')
    pe = struct.unpack_from('<I', img, 0x3C)[0]
    ts = struct.unpack_from('<I', img, pe + 8)[0]
    open(sys.argv[2], 'wb').write(img)
    print('escrito %s: %d bytes, MZ/PE, timestamp %d, sha1 %s' % (sys.argv[2], len(img), ts, hashlib.sha1(img).hexdigest()))


if __name__ == '__main__':
    main()
