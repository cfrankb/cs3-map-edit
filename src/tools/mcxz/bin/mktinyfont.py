t = open('bitfont.bin', 'wb')

with open('data1.bin', 'rb') as s:
    data = s.read()
    n = 8
    chunks = [data[i:i+n] for i in range(0, len(data), n)]
    print(chunks)
    for c in chunks:
        sum = 0
        v = 1
        for i in c:
            sum += (i & 1) * v
            v += v
        print(sum)
        t.write(bytes([sum]))
