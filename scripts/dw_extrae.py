#!/usr/bin/env python3
"""dw_extrae.py ELF SALIDA.pkl -- vuelca el DWARF 2 de un ELF de PS3 (GCC 4.0.2 de SN/Sony) a un pickle compacto.

Contenido del pickle (dict):
  cus:   [{off, name, comp_dir, producer, lang, files:[ruta], ranges:[(lo,hi)], macro_off}]
  funcs: [{cu, q (nombre cualificado), link (decorado), file, line, lo, hi, decl (DIE de la declaracion),
           params:[(nombre, tipo)], locals:[(nombre, tipo)], inl:[(q, call_file, call_line)], art (artificial),
           ext (externa)}]
  types: [{cu, q, tag, size, file, line, members:[(nombre, off, tipo)], bases:[(tipo, off, virtual)],
           methods:[nombre]}]
  enums: [{cu, q, size, vals:[(nombre, valor)]}]
  gvars: [{cu, q, link, tipo, file, line, addr}]
Solo pyelftools. Los ficheros se dan por argumento.
Pasada a scripts/ en la ronda 78 (procedencia: docs/analisis/2026-10-21_carbonps3.md); sin rutas de la maquina.
"""
import os
import pickle
import sys
import time

from elftools.elf.elffile import ELFFile

if __name__ == '__main__' and (len(sys.argv) < 3 or sys.argv[1] in ('-h', '--help')):
    print(__doc__)
    sys.exit(0)

TIPOS_AGREGADO = ('DW_TAG_structure_type', 'DW_TAG_class_type', 'DW_TAG_union_type')
AMBITOS = TIPOS_AGREGADO + ('DW_TAG_namespace',)


def s(v):
    return v.decode('latin-1') if isinstance(v, bytes) else v


def uleb(b, i):
    r = sh = 0
    while True:
        c = b[i]
        i += 1
        r |= (c & 0x7F) << sh
        sh += 7
        if not c & 0x80:
            return r, i


def loc_const(v):
    """DW_AT_data_member_location: entero o bloque DW_OP_plus_uconst N."""
    if isinstance(v, int):
        return v
    if isinstance(v, list) and v and v[0] == 0x23:
        return uleb(bytes(v), 1)[0]
    return None


def main():
    elf, salida = sys.argv[1], sys.argv[2]
    f = ELFFile(open(elf, 'rb'))
    dw = f.get_dwarf_info()
    t0 = time.time()
    cus, funcs, types, enums, gvars = [], [], [], [], []
    for ci, cu in enumerate(dw.iter_CUs()):
        top = cu.get_top_DIE()
        a = top.attributes
        g = lambda k: s(a[k].value) if k in a else None  # noqa: E731
        lp = dw.line_program_for_CU(cu)
        files = []
        if lp:
            dirs = [s(d) for d in lp['include_directory']]
            for fe in lp['file_entry']:
                n = s(fe.name)
                if fe.dir_index and fe.dir_index <= len(dirs):
                    n = dirs[fe.dir_index - 1].rstrip('/\\') + '/' + n
                files.append(n)
        rng = []
        if 'DW_AT_low_pc' in a and 'DW_AT_high_pc' in a:
            rng.append((a['DW_AT_low_pc'].value, a['DW_AT_high_pc'].value))
        cus.append(dict(off=cu.cu_offset, name=g('DW_AT_name'), comp_dir=g('DW_AT_comp_dir'),
                        producer=g('DW_AT_producer'), lang=g('DW_AT_language'), files=files, ranges=rng,
                        macro_off=g('DW_AT_macro_info'), stmt=g('DW_AT_stmt_list')))
        base = cu.cu_offset
        # primera pasada: todos los DIE del CU (tag, nombre, padre, atributos utiles)
        info = {}
        orden = []

        def ref(die, k):
            at = die.attributes.get(k)
            if at is None:
                return None
            v = at.value
            if at.form in ('DW_FORM_ref_addr',):
                return v
            return v + base

        def recorre(die, padre):
            for ch in die.iter_children():
                at = ch.attributes
                nm = s(at['DW_AT_name'].value) if 'DW_AT_name' in at else None
                d = dict(tag=ch.tag, name=nm, parent=padre, die=ch)
                info[ch.offset] = d
                orden.append(ch.offset)
                if ch.has_children:
                    recorre(ch, ch.offset)
        recorre(top, None)

        qcache = {}

        def qual(off, prof=0):
            d = info.get(off)
            if d is None or prof > 20:
                return None
            die = d['die']
            for k in ('DW_AT_specification', 'DW_AT_abstract_origin'):
                r = ref(die, k)
                if r is not None and r in info and d['name'] is None:
                    return qual(r, prof + 1)
                if r is not None and r in info and d['tag'] == 'DW_TAG_subprogram':
                    return qual(r, prof + 1)
            nm = d['name']
            if nm is None:
                return None
            p = d['parent']
            while p is not None and p in info:
                pd = info[p]
                if pd['tag'] in AMBITOS or pd['tag'] == 'DW_TAG_subprogram':
                    pq = qcache.get(p)
                    if pq is None:
                        pq = qual(p, prof + 1)
                        qcache[p] = pq
                    if pq is None:
                        pq = '(anonymous namespace)' if pd['tag'] == 'DW_TAG_namespace' else '(anon)'
                    return pq + '::' + nm
                p = pd['parent']
            return nm

        tcache = {}

        def tname(off, prof=0):
            if off is None:
                return 'void'
            if off in tcache:
                return tcache[off]
            d = info.get(off)
            if d is None or prof > 12:
                return '?'
            die = d['die']
            t = d['tag']
            sub = ref(die, 'DW_AT_type')
            if t == 'DW_TAG_pointer_type':
                r = tname(sub, prof + 1) + '*'
            elif t == 'DW_TAG_reference_type':
                r = tname(sub, prof + 1) + '&'
            elif t == 'DW_TAG_const_type':
                r = 'const ' + tname(sub, prof + 1)
            elif t == 'DW_TAG_volatile_type':
                r = 'volatile ' + tname(sub, prof + 1)
            elif t == 'DW_TAG_array_type':
                dims = ''
                for ch in die.iter_children():
                    if ch.tag == 'DW_TAG_subrange_type':
                        ub = ch.attributes.get('DW_AT_upper_bound')
                        dims += '[%s]' % ((ub.value + 1) if ub is not None and isinstance(ub.value, int) else '')
                r = tname(sub, prof + 1) + dims
            elif t == 'DW_TAG_subroutine_type':
                r = 'fn(%s)' % tname(sub, prof + 1)
            elif t == 'DW_TAG_ptr_to_member_type':
                r = 'ptrmem'
            else:
                r = qual(off) or ('(anon %s)' % t.replace('DW_TAG_', ''))
            tcache[off] = r
            return r

        def fname(die, k):
            at = die.attributes.get(k)
            if at is None:
                return None
            i = at.value
            return files[i - 1] if 0 < i <= len(files) else None

        for off in orden:
            d = info[off]
            die = d['die']
            at = die.attributes
            t = d['tag']
            if t == 'DW_TAG_subprogram' and 'DW_AT_low_pc' in at:
                q = qual(off)
                link = None
                decl = off
                o = off
                for _ in range(4):
                    dd = info[o]['die']
                    if 'DW_AT_MIPS_linkage_name' in dd.attributes:
                        link = s(dd.attributes['DW_AT_MIPS_linkage_name'].value)
                        break
                    r = ref(dd, 'DW_AT_specification') or ref(dd, 'DW_AT_abstract_origin')
                    if r is None or r not in info:
                        break
                    o = r
                    decl = r
                fi = fname(die, 'DW_AT_decl_file')
                ln = at['DW_AT_decl_line'].value if 'DW_AT_decl_line' in at else None
                if fi is None and decl != off:
                    fi = fname(info[decl]['die'], 'DW_AT_decl_file')
                    dl = info[decl]['die'].attributes.get('DW_AT_decl_line')
                    ln = dl.value if dl is not None else ln
                params, locs, inl = [], [], []

                def hijos(dd):
                    for ch in dd.iter_children():
                        ca = ch.attributes
                        if ch.tag in ('DW_TAG_formal_parameter', 'DW_TAG_variable'):
                            nm = s(ca['DW_AT_name'].value) if 'DW_AT_name' in ca else None
                            r = ref(ch, 'DW_AT_abstract_origin')
                            if nm is None and r in info:
                                nm = info[r]['name']
                            ty = tname(ref(ch, 'DW_AT_type')) if 'DW_AT_type' in ca else (
                                tname(ref(info[r]['die'], 'DW_AT_type')) if r in info else '?')
                            (params if ch.tag == 'DW_TAG_formal_parameter' else locs).append((nm, ty))
                        elif ch.tag == 'DW_TAG_lexical_block':
                            hijos(ch)
                        elif ch.tag == 'DW_TAG_inlined_subroutine':
                            r = ref(ch, 'DW_AT_abstract_origin')
                            inl.append((qual(r) if r else None, fname(ch, 'DW_AT_call_file'),
                                        ca['DW_AT_call_line'].value if 'DW_AT_call_line' in ca else None))
                if die.has_children:
                    hijos(die)
                hi = at['DW_AT_high_pc'].value if 'DW_AT_high_pc' in at else None
                funcs.append(dict(cu=ci, q=q, link=link, file=fi, line=ln, lo=at['DW_AT_low_pc'].value, hi=hi,
                                  params=params, locals=locs, inl=inl,
                                  art='DW_AT_artificial' in info[decl]['die'].attributes,
                                  ext='DW_AT_external' in at or 'DW_AT_external' in info[decl]['die'].attributes))
            elif t in TIPOS_AGREGADO and 'DW_AT_byte_size' in at and 'DW_AT_declaration' not in at:
                mem, bases, mets = [], [], []
                for ch in die.iter_children():
                    ca = ch.attributes
                    if ch.tag == 'DW_TAG_member':
                        nm = s(ca['DW_AT_name'].value) if 'DW_AT_name' in ca else None
                        o = loc_const(ca['DW_AT_data_member_location'].value) if 'DW_AT_data_member_location' in ca else (0 if t == 'DW_TAG_union_type' else None)
                        ty = tname(ref(ch, 'DW_AT_type'))
                        bits = None
                        if 'DW_AT_bit_size' in ca:
                            bits = (ca['DW_AT_bit_size'].value, ca['DW_AT_bit_offset'].value if 'DW_AT_bit_offset' in ca else None)
                        mem.append((nm, o, ty, bits))
                    elif ch.tag == 'DW_TAG_inheritance':
                        o = loc_const(ca['DW_AT_data_member_location'].value) if 'DW_AT_data_member_location' in ca else None
                        bases.append((tname(ref(ch, 'DW_AT_type')), o, 'DW_AT_virtuality' in ca))
                    elif ch.tag == 'DW_TAG_subprogram':
                        mets.append(s(ca['DW_AT_name'].value) if 'DW_AT_name' in ca else None)
                types.append(dict(cu=ci, q=qual(off), tag=t, size=at['DW_AT_byte_size'].value,
                                  file=fname(die, 'DW_AT_decl_file'),
                                  line=at['DW_AT_decl_line'].value if 'DW_AT_decl_line' in at else None,
                                  members=mem, bases=bases, methods=mets))
            elif t == 'DW_TAG_enumeration_type' and 'DW_AT_declaration' not in at:
                vals = []
                for ch in die.iter_children():
                    if ch.tag == 'DW_TAG_enumerator':
                        vals.append((s(ch.attributes['DW_AT_name'].value), ch.attributes['DW_AT_const_value'].value))
                enums.append(dict(cu=ci, q=qual(off), size=at['DW_AT_byte_size'].value if 'DW_AT_byte_size' in at else None,
                                  vals=vals))
            elif t == 'DW_TAG_variable' and 'DW_AT_location' in at and d['parent'] is None or (
                    t == 'DW_TAG_variable' and 'DW_AT_location' in at and info.get(d['parent'], {}).get('tag') == 'DW_TAG_namespace'):
                lo = at['DW_AT_location'].value
                addr = None
                if isinstance(lo, list) and lo and lo[0] == 0x03:
                    addr = int.from_bytes(bytes(lo[1:9 if len(lo) >= 9 else 5]), 'big')
                r = ref(die, 'DW_AT_specification')
                q = qual(r) if r in info else qual(off)
                dd = info[r]['die'] if r in info else die
                link = s(dd.attributes['DW_AT_MIPS_linkage_name'].value) if 'DW_AT_MIPS_linkage_name' in dd.attributes else None
                ty = tname(ref(dd, 'DW_AT_type'))
                gvars.append(dict(cu=ci, q=q, link=link, tipo=ty, file=fname(dd, 'DW_AT_decl_file'),
                                  line=dd.attributes['DW_AT_decl_line'].value if 'DW_AT_decl_line' in dd.attributes else None,
                                  addr=addr))
        print('%3d %-60s %6d fn %6d tipos %7.1f s' % (ci, os.path.basename((cus[-1]['name'] or '').replace('\\', '/')),
                                                     sum(1 for x in funcs if x['cu'] == ci),
                                                     sum(1 for x in types if x['cu'] == ci), time.time() - t0),
              flush=True)
        info.clear()
        tcache.clear()
    pickle.dump(dict(cus=cus, funcs=funcs, types=types, enums=enums, gvars=gvars), open(salida, 'wb'))
    print('total: %d CU, %d funciones, %d tipos, %d enums, %d globales, %.0f s' % (
        len(cus), len(funcs), len(types), len(enums), len(gvars), time.time() - t0))


if __name__ == '__main__':
    main()
