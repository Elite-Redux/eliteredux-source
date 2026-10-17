#!/bin/sh
# Build Elite Redux in a container. Needs Docker (with BuildKit, the default since 23.0)
# or Podman, plus the proto submodule. See docs/docker-build.md.
set -eu

usage() {
    cat <<'EOF'
usage: docker/build.sh [--clean] [COMMAND [ARGS...]]

Commands:
  bps          build the ROM and EliteRedux.bps into out/ (default)
  rom          build EliteRedux.gba into out/
  headers      run the Kotlin codegen and put include/generated and asm/generated in the tree
  make [ARGS]  run make in your checkout inside the build image (incremental, like a local build)
  shell        open a shell in the build image with your checkout mounted
  image        only build the build image

Options:
  --clean      for bps/rom: discard the cached work tree and build the ROM from scratch

Environment:
  CONTAINER_ENGINE   docker or podman (default: docker if installed, else podman)
  JOBS               parallel make jobs (default: all CPUs)
  ER_BUILD_FLAGS     extra flags for `<engine> build`, e.g. "--network host"
  ER_RUN_FLAGS       extra flags for `<engine> run`
EOF
}

repo=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
platform=linux/amd64
builder_image=eliteredux-builder:local
incremental=1

while [ $# -gt 0 ]; do
    case "$1" in
        --clean) incremental=0; shift ;;
        -h|--help) usage; exit 0 ;;
        *) break ;;
    esac
done
command=${1:-bps}
[ $# -gt 0 ] && shift

engine=${CONTAINER_ENGINE:-}
if [ -z "${engine}" ]; then
    if command -v docker >/dev/null 2>&1; then engine=docker
    elif command -v podman >/dev/null 2>&1; then engine=podman
    else echo "Neither docker nor podman was found" >&2; exit 1
    fi
fi

if [ ! -f "${repo}/proto/SpeciesList.proto" ]; then
    echo "proto/ is empty. Fetch the er-config submodule first:" >&2
    echo "  git submodule update --init proto" >&2
    exit 1
fi

build() {
    # shellcheck disable=SC2086
    "${engine}" build ${ER_BUILD_FLAGS:-} \
        --platform "${platform}" \
        --file "${repo}/docker/Dockerfile" \
        --build-arg INCREMENTAL="${incremental}" \
        --build-arg JOBS="${JOBS:-}" \
        "$@" \
        "${repo}"
}

export_target() {
    target=$1
    dest=$2
    build --target "${target}" --output "type=local,dest=${dest}"
}

run_in_builder() {
    build --target builder --tag "${builder_image}"
    if [ "${engine}" = podman ] && [ "$(podman info --format '{{.Host.Security.Rootless}}')" = true ]; then
        # Rootless Podman: map your user to the same uid inside the container.
        user_flags="--userns=keep-id"
    else
        user_flags="--user $(id -u):$(id -g)"
    fi
    tty=
    if [ -t 0 ] && [ -t 1 ]; then tty=-it; fi
    # shellcheck disable=SC2086
    exec "${engine}" run --rm ${tty} --init ${ER_RUN_FLAGS:-} ${user_flags} \
        --platform "${platform}" \
        --env HOME=/tmp \
        --env JOBS \
        --volume "${repo}:/src:z" \
        --workdir /src \
        "${builder_image}" "$@"
}

case "${command}" in
    bps)
        export_target bps-artifact "${repo}/out"
        echo "Built out/EliteRedux.gba and out/EliteRedux.bps"
        ;;
    rom)
        export_target rom-artifact "${repo}/out"
        echo "Built out/EliteRedux.gba"
        ;;
    headers)
        mkdir -p "${repo}/out"
        tmp=$(mktemp -d "${repo}/out/.headers.XXXXXX")
        trap 'rm -rf "${tmp}"' EXIT
        export_target headers-artifact "${tmp}"
        for dir in include/generated asm/generated; do
            rm -rf "${repo:?}/${dir}"
            mkdir -p "$(dirname "${repo}/${dir}")"
            cp -R "${tmp}/${dir}" "${repo}/${dir}"
        done
        echo "Updated include/generated and asm/generated"
        ;;
    make)
        run_in_builder sh -c 'er-prepare-tree /src && exec make -j"${JOBS:-$(nproc)}" "$@"' make "$@"
        ;;
    shell)
        run_in_builder sh -c 'er-prepare-tree /src && exec bash'
        ;;
    image)
        build --target builder --tag "${builder_image}"
        ;;
    *)
        usage >&2
        exit 2
        ;;
esac
