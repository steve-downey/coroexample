This repo currently assumes that you have clang version 14 available on your path as clang++-14 and a current libstdc++.

Given that, you should be able to build by
```sh
make
```

and run the standard example by

```sh
make run
```

This currently uses the toolchain file in etc/ that specifies clang-14 with libstdc++ as the the standard C++ library.

The Makefile is also executable, and the repository can be built via
```sh
./build
```
`
