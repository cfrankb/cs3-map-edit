

defs = []
conv = []

with open('cs3gra1.map') as s:
    lines = s.read().split('\n')
    for line in lines:
        line = line.strip()
        if not line:
            break
        print(line)

        a = line.split(' ')
        val = a[0]
        name = a[1].upper()
        # tfile.write(f'#define CS3_{name} 0x{val}\n')
        defs.append(f'#define CS3_{name} 0x{val}')
        if len(a) == 3:
            tile = a[2].upper()
        else:
            tile = a[1].upper()

        conv.append(f'TILES_{tile}')

with open('cs3gra.h', 'w') as t:
    t.write('\n'.join(defs))

with open('cs3gra.cpp', 'w') as t:
    # t.write('#include "tilesdata.h"\n' + '#include "cs3gra.h"\n\n')
    t.write(',\n'.join(conv))
