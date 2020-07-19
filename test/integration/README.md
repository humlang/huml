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

The simplest way to run the integration tests is by executing the `run_all.sh` script in this directory.

For more fine grained control, the tests expect the path to the `huml` binary as environment variable.
Also, they expect the path the tests.
An example execution (if `huml` is installed in home dir and the binary in a `build` directory) is:

```sh
export HUML_PATH="$HOME/huml/build/"
export HUML_TEST="$HOME/huml/test/"

cd "$HUML_TEST"/integration/parsing/
./classic_programs.bats
```


