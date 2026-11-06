# Methodology

Inventory sources files (*.h, *.cpp), such as `main.cpp` and those others in folder `src/` and it's subfolders. Ignore files in `src/runtime` as they are to be covered in another project.

## tests
create tests for each of those files, Compare against exists hash to determined if the file has changed. Use `Qt Test` framework.

- unit tests
- functional tests
- per function tests
- elaborate tests that requires mocking
- aim for full coverage

tests that cannot run on github-ci will be run locally only. plan target splitting accordingly. 

For use Qt AUTOUIC tool to handle ui. Testing of the later is limited to local.

isolate the content of `src/runtime/` as a separate library that can be linked with the main target and test targets.

## static data

there are some files that can be used for testing in `tests/in` and the subfolders.

the refs are intended to compare conversion between files with different encodings (obl3, obl4, obl5 etc) and are out of scope for testing at this stage, `src/runtime/shared/` contained it's own png library independant of the GDI.


## instructions

track hash (`git hash-object`) for those files and updated `tests/TRACKER.md` for each file that is covered.

update test results in `tests/STATUS.md`
create a target to bind these tests in the root `CMakeLists.txt` file.
modifiy CMakeLists.txt to enable testing as needed.
add github-ci hook to run the tests on all platforms already present

you'll manage `tests/README.md`.

## advice on improvements

log recommendations in `tests/REVIEW.md`.