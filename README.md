# Jayceon
A simple [JSON](https://json.org) parser I wrote in a few days in ANSI C. You can learn how to use it by reading the header file. The project is fully public domain (under the [unlicense](https://unlicense.org)). There is still improvement to be had (currently does not support serious serialization or data construction, just parsing and accessing data).

## Building
Simply, compile `jayceon.c` with `jayceon.h` in its include path. If the compiler you're using does not want to compile it for some reason, please tell me, because the library successfully compiles under `-Wall -Wextra -Werror -pedantic -ansi` with GCC, and the whole "written in ANSI C" thing is so that it can be used on as many platforms as hopefully possible.

## Planned features
I plan to add parsing through a callback, serialization and data construction in some future version of the library.
