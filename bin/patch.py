path = "src/mapedit.pro"

with open(path) as sfile:
    data = sfile.read()

find = "DEFINES += USE_HUNSPELL=1"

data = data.replace(find, "#" + find)

with open(path, "w") as tfile:
    tfile.write(data)
