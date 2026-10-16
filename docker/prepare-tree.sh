#!/bin/sh
# Put the image's pinned Poryscript and codegen downloads into the source tree, so make
# never has to fetch them. Existing files are left alone.
set -eu

tree=${1:-.}

if [ ! -x "${tree}/tools/poryscript/poryscript" ]; then
    mkdir -p "${tree}/tools/poryscript"
    cp -p /opt/poryscript/* "${tree}/tools/poryscript/"
fi

for file in protoc protoc.zip protobuf-java.jar protobuf-kotlin.jar; do
    [ -e "${tree}/tools/codegen/${file}" ] || cp -p "/opt/codegen-deps/${file}" "${tree}/tools/codegen/"
done
