#!/bin/sh

# Search in the script folder
pushd "$(dirname $0)" >/dev/null
CWD="$(pwd -P)"
popd >/dev/null
FILES='src/judynode.cc src/judynode.h'

# The file format in accordance with the style defined in .astylerc
astyle -v --options='.astylerc' ${FILES} || (echo 'astyle failed'; exit 1);

# To correct this, the issuance dos2unix on each file
# sometimes adds in Windows as a string-endins (\r\n).
dos2unix --quiet ${FILES} || (echo 'dos2unix failed'; exit 2);
