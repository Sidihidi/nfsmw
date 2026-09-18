// globala.bun embebido: el paquete de recursos global que el juego lleva dentro del ejecutable (fallback cuando no
// esta en disco): simbolo bin_globala_bun, 89.716 bytes, en el .data de zPlatform. Son DATOS DEL JUEGO y no se
// distribuyen: zPlatform es NonMatching, asi que el DOL enlaza el objeto que el build extrae de tu propio binario,
// con estos bytes dentro. Aqui solo queda la declaracion.
extern "C" unsigned char bin_globala_bun[89716];
