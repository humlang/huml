SETUP
=====

Install bats:

```sh
git clone https://github.com/sstephenson/bats.git
cd bats
sudo ./install.sh /usr/local
```

HOWTO
=====

In integration expects the path to the hx-lang binary as environment variable.
Also, it expects the path the tests.
An example execution (if hx-lang is installed in home dir and the binary in a `build` directory) is:

```sh
export HX_LANG_PATH="$HOME/hx-lang/build/"
export HX_LANG_TEST="$HOME/hx-lang/test/"

cd "$HX_LANG_TEST"/integration/parsing/
./classic_programs.bats
```


