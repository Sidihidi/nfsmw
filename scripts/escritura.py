"""Escritura segura de ficheros de config (ronda 56).

`open(ruta, 'w')` vacia el fichero ANTES de escribir: si el disco se llena a mitad (ronda 55, symbols.txt de X360)
el fichero queda vacio o cortado. Aqui se escribe en un temporal del MISMO directorio y se cambia con os.replace,
que es atomico: o queda el fichero nuevo entero o el de antes intacto.

    from escritura import escribe, abre, entero
    escribe(ruta, texto, encoding='utf-8', newline='\\n')     # str -> modo 'w'; bytes -> modo 'wb'
    entero(ruta, 'w', encoding='utf-8').write(texto)          # = open(...).write(texto), de una vez
    with abre(ruta, 'w', encoding='utf-8') as fh:             # para json.dump, csv, bucles de fh.write
        json.dump(d, fh)

Los argumentos son los de open(); el contenido escrito es byte a byte el mismo que con open(ruta, modo, ...).
"""
import contextlib
import os
import time


@contextlib.contextmanager
def abre(ruta, modo='w', **kw):
    if 'w' not in modo:
        raise ValueError('escritura.abre solo escribe ficheros enteros (modo %r)' % modo)
    ruta = os.fspath(ruta)
    tmp = '%s.tmp%d' % (ruta, os.getpid())
    try:
        with open(tmp, modo, **kw) as fh:
            yield fh
            fh.flush()
            os.fsync(fh.fileno())
        for intento in range(5):
            try:
                os.replace(tmp, ruta)
                break
            except PermissionError:          # Windows: otro proceso tiene el destino abierto un instante
                if intento == 4:
                    raise
                time.sleep(0.2 * (intento + 1))
    finally:
        if os.path.exists(tmp):
            os.remove(tmp)


def escribe(ruta, datos, modo=None, **kw):
    if modo is None:
        modo = 'wb' if isinstance(datos, (bytes, bytearray)) else 'w'
    with abre(ruta, modo, **kw) as fh:
        fh.write(datos)


class entero:
    """Sustituto directo de `open(ruta, modo, ...).write(datos)` cuando el fichero se escribe de UNA vez:
    `entero(ruta, modo, ...).write(datos)`. Cada write() reescribe el fichero ENTERO (no acumula)."""

    def __init__(self, ruta, modo='w', **kw):
        self.ruta, self.modo, self.kw = ruta, modo, kw

    def write(self, datos):
        escribe(self.ruta, datos, self.modo, **self.kw)
        return len(datos)
