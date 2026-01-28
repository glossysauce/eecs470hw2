# gen_icache.py
# generates many big functions to probe icache size

N_FUNCS = 1024  # number of functions
NOP_BLOCKS = 8 # controls size of each function

with open("icache_funcs.c", "w") as f:
    f.write('#include <stdint.h>\n#include <stddef.h>\n\n')

    f.write(
        '#define DO_8_NOPS \\\n'
        '  asm volatile("nop\\n'
        'nop\\n'
        'nop\\n'
        'nop\\n'
        'nop\\n'
        'nop\\n'
        'nop\\n'
        'nop\\n");\n\n'
    )

    f.write(
        '#define DO_64_NOPS \\\n'
        '  DO_8_NOPS DO_8_NOPS DO_8_NOPS DO_8_NOPS \\\n'
        '  DO_8_NOPS DO_8_NOPS DO_8_NOPS DO_8_NOPS\n\n'
    )

    for i in range(N_FUNCS):
        f.write(f'__attribute__((noinline)) void f{i}(volatile uint64_t *s) {{\n')
        for _ in range(NOP_BLOCKS):
            f.write('    DO_64_NOPS\n')
        f.write(f'    *s += {i};\n}}\n\n')

    f.write('void (*ALL[])(volatile uint64_t*) = {\n')
    for i in range(N_FUNCS):
        f.write(f'    f{i},\n')
    f.write('};\n')

    f.write(f'const size_t ALL_N = {N_FUNCS};\n')
    