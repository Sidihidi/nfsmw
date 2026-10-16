#!/usr/bin/env python3
"""ctor360.py -- constructores y destructores de Xbox 360 cuyo NOMBRE contradice su CODIGO (la vtable que escriben).

Un constructor escribe en `this+0` la direccion de la vtable de SU clase (la ultima que escribe: antes van las de
las bases expandidas); un destructor, la primera (solo si escribe dos o mas: la escritura de la suya, seguida sin
nada en medio de la de la base, la quita el optimizador y queda solo la de la base, `IUnknown` en las interfaces). La clase de una vtable del original se lee de sus huecos: el nombre
`??_7Clase@@6B@` si symbols.txt lo tiene, o el destructor de borrado (`??_E`/`??_G`) o `GetTypeName`/`GetTypeInfo`
de la clase en sus huecos, siempre que los metodos de esa clase en los huecos sean mas que los de la clase del nombre
(esos nombres heredados tambien pueden estar mal: la vtable de CSTATE_Base lleva un `GetTypeName@SFXCTL_Helicopter`). Si el nombre de la funcion dice otra clase, el nombre es FALSO.

Ejemplo (ronda 53): `??0CARSFX_TrafficWoosh@@QAA@XZ` (0x8220C1F8) escribe la vtable 0x82076AAC, cuyos huecos son
`??_ECARSFX_WindWeather`, `GetTypeName@CARSFX_WindWeather`...: es el constructor de CARSFX_WindWeather. En zEAXSound2
habia una cadena entera de constructores corridos un puesto.

Las factorias `?CreateObject@Clase@@S...` (el sonido: SndBase) se miran igual: la unica vtable que cargan es la del
objeto que crean (0x82218658 se llamaba `CreateObject@SFXObj_TruckFX` y crea un `SFXObj_Pathfinder`).

Una factoria que no expande el constructor toma la vtable del constructor al que llama.

Nombre nuevo: el `??0`/`??1`/`?CreateObject@` de esa clase que DEFINE nuestro objeto LTCG de la unidad con el mismo esqueleto (codigo
con los campos reubicados a cero); si no hay ninguno igual, el mismo nombre con la clase cambiada (solo si el resto
del nombre no lleva referencias atras, `@0`..`@9`, que cambiarian de sentido) cuando la prueba es FUERTE: la vtable
tiene el nombre PRIMARIO `??_7Clase@@6B@` (las secundarias `6BBase@` engannan: `??_7DrawVehicle@@6BIServiceable@Sim@@@`
es la de VehicleBehavior), sus huecos llevan dos o mas metodos de esa clase y ninguno de la clase del nombre, y la
funcion no casa hoy al 100 % (nunca se pierde nada). Con --sin-codigo, siempre (solo para mirar). Un nombre no
puede repetirse en una unidad: si ya lo lleva otra funcion, esa tambien tiene que cambiar (punto fijo); si la otra
tambien es falsa y no tiene nombre nuevo, pasa a fn_ (--anonimizar). Despues: la rutina de nombres de X360.

    python scripts/ctor360.py [--detalle] [--sin-codigo] [--anonimizar] [--aplicar]
"""
import collections
import glob
import json
import os
import re
import struct
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if not os.path.isdir(os.path.join(RAIZ, 'config', 'EUROPEGERMILESTONE')):
    RAIZ = os.environ.get('NFSMW_RAIZ', os.getcwd())  # copia de trabajo fuera del arbol: la raiz es el directorio actual
sys.path.insert(0, os.path.join(RAIZ, 'scripts'))
import nombra_grafo as ng  # noqa: E402
from escritura import entero  # noqa: E402

V = 'EUROPEGERMILESTONE'
BASE = 0x82000000


class Exe:
    def __init__(self, ruta):
        d = open(ruta, 'rb').read()
        self.d = d
        pe = struct.unpack_from('<I', d, 0x3c)[0]
        nsec = struct.unpack_from('<H', d, pe + 6)[0]
        opt = struct.unpack_from('<H', d, pe + 20)[0]
        self.secs = []
        o = pe + 24 + opt
        for _ in range(nsec):
            nombre = d[o:o + 8].rstrip(b'\0').decode('latin-1')
            vs, va, rs, rp = struct.unpack_from('<IIII', d, o + 8)
            self.secs.append((nombre, BASE + va, max(vs, rs), rp, rs))
            o += 40

    def seccion(self, a):
        for n, va, tam, rp, rs in self.secs:
            if va <= a < va + tam:
                return n
        return None

    def palabra(self, a):
        for n, va, tam, rp, rs in self.secs:
            if va <= a < va + rs:
                return struct.unpack_from('>I', self.d, rp + a - va)[0]
        return None


def clase_de_nombre(n):
    """'??0Clase@Esp@@QAA@XZ' -> ('Clase@Esp@', 'QAA@XZ'); None si no es ctor/dtor/metodo decorado."""
    m = re.match(r'^\?(?:\?[01]|\?_[EG]|[A-Za-z_]\w*@)((?:[A-Za-z_]\w*@)+)@(.*)$', n)
    if not m:
        return None
    return m.group(1), m.group(2)


def clase_de_vtable(exe, a, nombre_en, P):
    """(clase, como, todos): la clase de la vtable y cuantos huecos llevan metodos de cada clase."""
    todos = collections.Counter()
    for k in range(64):
        if k and (a + 4 * k) in nombre_en:
            break
        w = exe.palabra(a + 4 * k)
        if w is None or exe.seccion(w) != '.text':
            break
        for n in nombre_en.get(w, [])[:1]:
            cn = clase_de_nombre(n)
            if cn:
                todos[cn[0]] += 1
    for n in nombre_en.get(a, []):
        m = re.match(r'^\?\?_7((?:[A-Za-z_]\w*@)+)@6B', n)
        if m:
            return m.group(1), 'vtable', todos
    votos = collections.Counter()
    for k in range(64):
        if k and (a + 4 * k) in nombre_en:
            break  # otra vtable (con nombre o lbl_) empieza aqui
        w = exe.palabra(a + 4 * k)
        if w is None or exe.seccion(w) != '.text':
            break
        for n in nombre_en.get(w, []):
            m = re.match(r'^\?\?_[EG]((?:[A-Za-z_]\w*@)+)@', n) or                 re.match(r'^\?Get(?:TypeName|TypeInfo)@((?:[A-Za-z_]\w*@)+)@', n)
            if m:
                votos[m.group(1)] += 1
    if len(votos) == 1:
        return next(iter(votos)), 'huecos', todos
    return None, ('ambigua %s' % dict(votos)) if votos else 'sin pista', todos


def vtables_escritas(exe, ini, tam):
    """Direcciones (en .rdata) escritas en this+0, en orden. this = r3 al entrar, y las copias con mr."""
    this = {3}
    hi = {}
    full = {}
    out = []
    for a in range(ini, ini + tam, 4):
        w = exe.palabra(a)
        op = w >> 26
        rd, ra = (w >> 21) & 31, (w >> 16) & 31
        imm = w & 0xFFFF
        simm = imm - 0x10000 if imm & 0x8000 else imm
        if op == 15 and ra == 0:        # lis
            hi[rd] = (imm << 16) & 0xFFFFFFFF
            full.pop(rd, None)
            this.discard(rd)
        elif op == 14 and ra in hi:     # addi rd, ra, simm
            full[rd] = (hi[ra] + simm) & 0xFFFFFFFF
            if rd != ra:
                hi.pop(rd, None)
            this.discard(rd)
        elif op == 36:                  # stw rs, d(ra)
            if ra in this and simm == 0 and rd in full and exe.seccion(full[rd]) == '.rdata':
                out.append(full[rd])
        elif op == 31 and ((w >> 1) & 0x3FF) == 444 and rd == ((w >> 11) & 31):  # mr ra, rs
            if rd in this:
                this.add(ra)
            else:
                this.discard(ra)
            hi.pop(ra, None)
            full.pop(ra, None)
        elif op == 18 and w & 1:        # bl: volatiles fuera
            for r in [0] + list(range(3, 13)):
                this.discard(r)
                hi.pop(r, None)
                full.pop(r, None)
        else:
            # cualquier otra escritura en rd la invalida (aproximado: formas D)
            if op in (32, 33, 34, 35, 40, 41, 42, 43, 7, 8, 12, 13, 14, 15, 24, 25, 28, 29, 21, 20):
                this.discard(rd if op not in (24, 25, 28, 29, 21, 20) else ra)
                k = rd if op not in (24, 25, 28, 29, 21, 20) else ra
                hi.pop(k, None)
                full.pop(k, None)
    return out


def vtables_cargadas(exe, ini, tam):
    """Direcciones de .rdata que la funcion forma con lis/addi y que empiezan por un puntero a .text (vtables)."""
    hi = {}
    out = []
    for a in range(ini, ini + tam, 4):
        w = exe.palabra(a)
        op = w >> 26
        rd, ra = (w >> 21) & 31, (w >> 16) & 31
        imm = w & 0xFFFF
        simm = imm - 0x10000 if imm & 0x8000 else imm
        if op == 15 and ra == 0:
            hi[rd] = (imm << 16) & 0xFFFFFFFF
        elif op == 14 and ra in hi:
            v = (hi[ra] + simm) & 0xFFFFFFFF
            if exe.seccion(v) == '.rdata' and exe.seccion(exe.palabra(v) or 0) == '.text':
                out.append(v)
    return out


def llamadas(exe, ini, tam):
    out = []
    for a in range(ini, ini + tam, 4):
        w = exe.palabra(a)
        if w >> 26 == 18 and w & 1:
            li = w & 0x03FFFFFC
            if li & 0x02000000:
                li -= 0x04000000
            out.append((li if w & 2 else a + li) & 0xFFFFFFFF)
    return out


FACTORIA = re.compile(r'^\?CreateObject@')


def main():
    a = sys.argv[1:]
    P = ng.X360()
    exe = Exe(os.path.join(RAIZ, 'orig', V, 'NFS.exe'))
    nombre_en = collections.defaultdict(list)
    tam = {}
    for i, l in enumerate(P.lineas):
        m = re.match(r'(\S+) = (\.\w+):0x([0-9A-Fa-f]+);.*?(?:size:0x([0-9A-Fa-f]+))?\s*$', l)
        if m:
            d = int(m.group(3), 16)
            nombre_en[d].append(m.group(1))
            if m.group(2) == '.text' and m.group(4):
                tam[d] = int(m.group(4), 16)
    # nuestros objetos LTCG por unidad: {unidad: {nombre: esqueleto}} y el original troceado
    nuestros = {}
    orig = {}
    base_s = os.path.join(RAIZ, 'build', V, 'ltcg', 'obj')
    base_o = os.path.join(RAIZ, 'build', V, 'obj')
    for p in glob.glob(os.path.join(base_s, '**', '*.obj'), recursive=True):
        rel = os.path.relpath(p, base_s)
        un = rel.replace(os.sep, '/')[:-4]
        nuestros[un] = {n: P.esqueleto(*v) for n, v in ng.Coff(p).funciones().items()}
        o = os.path.join(base_o, rel)
        if os.path.exists(o):
            orig[un] = {n: P.esqueleto(*v) for n, v in ng.Coff(o).funciones().items()}

    pct = {}
    rep = json.load(open(os.path.join(RAIZ, 'build', 'versiones', V + '.json')))
    for u in rep['units']:
        for f in u.get('functions') or []:
            pct[(u['name'].split('/')[-1], f['name'])] = f.get('fuzzy_match_percent')
    falsos = {}     # dir -> (nombre, clase vista, nuevo o None, motivo)
    vistos = 0
    vt_de_clase = {}
    for d0, ns in nombre_en.items():
        for x in ns:
            m = re.match(r'^\?\?_7((?:[A-Za-z_]\w*@)+)@6B@$', x)
            if m:
                vt_de_clase[m.group(1)] = d0
    for d in sorted(P.texto):
        for n in nombre_en.get(d, []):
            if not (n.startswith(('??0', '??1')) or FACTORIA.match(n)) or d not in tam:
                continue
            cn = clase_de_nombre(n)
            if not cn:
                continue
            if FACTORIA.match(n):
                vts = sorted(set(vtables_cargadas(exe, d, tam[d])))
                if not vts:  # sin el constructor expandido: la vtable que escribe el constructor al que llama
                    for c in set(llamadas(exe, d, tam[d])):
                        if c in tam and not any(x.startswith('?AllocateMemory@') for x in nombre_en.get(c, [])):
                            v2 = vtables_escritas(exe, c, tam[c])
                            if v2:
                                vts.append(v2[-1])
                    vts = sorted(set(vts))
                if len(vts) != 1:
                    continue  # la factoria tiene que crear UNA clase
            else:
                vts = vtables_escritas(exe, d, tam[d])
                if not vts or (n.startswith('??1') and len(vts) < 2):
                    continue  # un destructor con una sola escritura: la suya puede haberla quitado el optimizador
            vt = vts[0] if n.startswith('??1') else vts[-1]
            clase, como, todos = clase_de_vtable(exe, vt, nombre_en, P)
            vistos += 1
            if clase is not None and como == 'huecos' and todos[cn[0]] >= todos[clase]:
                clase = cn[0]  # los huecos con metodos de la clase del nombre pesan tanto o mas: la pista es falsa
            if clase is None and cn[0] in vt_de_clase and vt_de_clase[cn[0]] != vt and                     todos[cn[0]] == 0 and pct.get(((P.unidad_de(d) or '').split('/')[-1], n)) != 100.0:
                # la vtable de la clase del nombre tiene nombre y es OTRA, y en los huecos de la que escribe no hay
                # ningun metodo de esa clase: el nombre es falso aunque no se sepa el bueno (fn_ con --anonimizar)
                falsos[d] = (n, '?', None, 'su clase tiene la vtable %08X y esta escribe %08X' % (vt_de_clase[cn[0]], vt),
                             P.unidad_de(d))
                continue
            if clase is None or clase == cn[0]:
                if '--detalle' in a and clase is None:
                    print('?? %08X %-50s vt %08X %s' % (d, n[:50], vt, como))
                continue
            un = P.unidad_de(d)
            nuevo, motivo = None, 'sin igual nuestro'
            if un in nuestros and un in orig and n in orig[un]:
                e = orig[un][n]
                pref = '?CreateObject@' if FACTORIA.match(n) else n[:3]
                cand = [x for x, es in nuestros[un].items() if es == e and x.startswith(pref) and
                        (clase_de_nombre(x) or ('',))[0] == clase]
                iguales = [x for x, es in nuestros[un].items() if es == e]
                if len(iguales) > 1:
                    cand = []  # codigo de familia (constructores de interfaz, plantillas): el codigo no prueba nada
                    motivo = 'codigo igual al de %d funciones nuestras' % len(iguales)
                if len(cand) == 1:
                    nuevo, motivo = cand[0], 'codigo igual (%s)' % como
                elif len(cand) > 1:
                    motivo = 'varios iguales'
                elif FACTORIA.match(n) and len(iguales) <= 1:
                    # la pista del destructor de borrado tambien puede ser un nombre falso: otra clase con dos o mas
                    # metodos en los huecos y UNA factoria nuestra igual (solo factorias: en un constructor, una
                    # derivada que no redefine nada llena los huecos con metodos de la base)
                    otras = []
                    for c, k in todos.most_common():
                        if k < 2 or c in (cn[0], clase):
                            continue
                        cc = [x for x, es in nuestros[un].items() if es == e and x.startswith(pref) and
                              (clase_de_nombre(x) or ('',))[0] == c]
                        if len(cc) == 1:
                            otras.append((c, cc[0]))
                    if len(otras) == 1:
                        clase, nuevo = otras[0]
                        motivo = 'codigo igual (huecos con %d metodos de la clase)' % todos[clase]
            elif un not in nuestros:
                motivo = 'unidad sin objeto nuestro'
            primaria = como == 'vtable' and ('??_7%s@6B@' % clase) in nombre_en.get(vt, [])
            fuerte = (primaria and todos[clase] >= 2 and todos[cn[0]] == 0 and
                      pct.get(((un or '').split('/')[-1], n)) != 100.0)
            if nuevo is None and (fuerte or '--sin-codigo' in a) and not re.search(r'@[0-9]', cn[1]) and \
                    clase.count('@') == cn[0].count('@'):
                pref = '?CreateObject@' if FACTORIA.match(n) else n[:3]
                nuevo = pref + clase + '@' + cn[1]
                motivo = ('vtable %s con %d metodos de la clase, sin codigo igual' % ('??_7' + clase + '@6B@', todos[clase])
                          if fuerte else 'clase de la vtable (%s), sin codigo (--sin-codigo)' % como)
            falsos[d] = (n, clase, nuevo, motivo, un)

    # punto fijo: el nombre nuevo no puede quedar repetido en la unidad
    def holders(nombre, un):
        return [x for x in P.por_nombre.get(nombre, []) if x in P.texto and P.unidad_de(x) == un]

    acept = {d: v for d, v in falsos.items() if v[2]}
    anon = set()
    while True:
        malos = set()
        destino = collections.Counter((v[4], v[2]) for d, v in acept.items())
        for d, v in acept.items():
            if destino[(v[4], v[2])] > 1:
                malos.add(d)
                continue
            for h in holders(v[2], v[4]):
                if h == d or h in acept:
                    continue
                if h in falsos and '--anonimizar' in a and pct.get(((v[4] or '').split('/')[-1], falsos[h][0])) != 100.0:
                    anon.add(h)
                    continue
                malos.add(d)
        if not malos:
            break
        for d in malos:
            acept.pop(d)
    anon = {h for h in anon if h not in acept and any(holders(v[2], v[4]).count(h) for v in acept.values())}
    print('ctor/dtor/factoria con vtable vista: %d; nombre contradicho: %d; con nombre nuevo: %d; aceptados: %d; a fn_: %d'
          % (vistos, len(falsos), sum(1 for v in falsos.values() if v[2]), len(acept), len(anon)))
    for d, (n, clase, nuevo, motivo, un) in sorted(falsos.items()):
        marca = 'OK ' if d in acept else ('FN ' if d in anon else '-- ')
        print('%s%08X %-12s %-55s clase %-30s -> %s  [%s]' % (marca, d, (un or '-').split('/')[-1], n[:55], clase[:30],
                                                             (nuevo or '-')[:70], motivo))
    if '--aplicar' in a:
        lin = P.lineas
        cambios = [(d, falsos[d][0], acept[d][2]) for d in acept] + [(d, falsos[d][0], 'fn_%08X' % d) for d in anon]
        for d, viejo, nuevo in cambios:
            i = [e for e in P.por_dir[d] if e[0] == viejo and e[1] == '.text'][0][2]
            assert lin[i].startswith(viejo + ' = .text:0x%08X;' % d), lin[i]
            lin[i] = nuevo + lin[i][len(viejo):]
        entero(os.path.join(RAIZ, 'config', V, 'symbols.txt'), 'w', encoding='utf-8', newline='\n').write('\n'.join(lin))
        print('aplicado', len(cambios))


if __name__ == '__main__':
    main()
