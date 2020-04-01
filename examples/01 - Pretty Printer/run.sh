# compile the example
cmake . && make

# run tblgen
unameOut="$(uname -s)"
case "${unameOut}" in
    Linux*)     extension=so;;
    Darwin*)    extension=dylib;;
    *)          extension=so;;
esac

tblgen Example01.tg -pretty-print libexample01.${extension}