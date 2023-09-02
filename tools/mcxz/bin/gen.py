import ntpath
import os

def prepare_deps(deps, fname):
    lines = []
    basename = ntpath.basename(fname)
    name = basename.replace('.cpp', '')
    fname_h =fname.replace('.cpp', '.h')
    ref = fname
    if os.path.isfile(fname_h):
        ref += f' {fname_h}'
    lines.append(f'$(BPATH)/{name}$(EXT): {ref}')
    lines.append(f'\t$(CXX) $(CXXFLAGS) -c $< -o $@')
    deps.append(f'{name}')
    return '\n'.join(lines)

def get_deps_blocks():
    deps=[]
    deps_blocks = ["all: $(TARGET)"]
    files = [
        'src/main.cpp',
         '../../map.cpp',
         'src/tileset.cpp',
         '../../shared/FileWrap.cpp',
         '../../shared/Frame.cpp',
         '../../shared/FrameSet.cpp',
         '../../shared/PngMagic.cpp',
         '../../shared/DotArray.cpp',
         '../../shared/helper.cpp'
    ]

    for f in files:
        deps_blocks.append(prepare_deps(deps, f))

    objs = ' '.join(f'$(BPATH)/{x}$(EXT)' for x in deps)
    lines = []
    lines.append(f'$(TARGET): $(DEPS)')
    lines.append(f'\t$(CXX) $(CXXFLAGS) $(DEPS) $(LIBS) -o $@')
    deps_blocks.append('\n'.join(lines))
    lines = []
    lines.append('clean:')
    lines.append("\trm -f $(BPATH)/*")
    deps_blocks.append('\n'.join(lines))
    return deps_blocks, objs

def main():
    vars = [
        'CXX=g++',
        'CXXFLAGS=-g  -DQT_NOT_WANTED',
        'LIBS=-lm -lz',
        'BPATH=build',
        'EXT=.o',
        'TARGET=$(BPATH)/mcxz'
    ]

    deps_blocks, objs = get_deps_blocks()
    vars.append(f'DEPS={objs}')

    with open('makefile','w') as tfile:
        tfile.write('\n'.join(vars) + "\n\n")
        tfile.write('\n\n'.join(deps_blocks))

main()