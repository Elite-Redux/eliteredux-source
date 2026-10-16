# Building with Docker or Podman

`docker/build.sh` builds Elite Redux in a container, so you don't need an ARM toolchain,
a JDK, Kotlin, Poryscript or protoc on your machine. The image pins the same tools CI uses
(Arm GNU Toolchain 14.2, Kotlin, Poryscript 3.5.2, protoc 29.2), and every download is
checked against a SHA-256.

## Requirements

- Docker 23 or newer (BuildKit is the default there), or Podman 4 or newer
- The er-config submodule:

  ```sh
  git submodule update --init proto
  ```

  Only fetch `proto`: `--recursive` trips over a leftover `eliteredux-darky` entry.

The image is `linux/amd64`. On Apple Silicon it runs under emulation, so the first build is slow.

## Building

```sh
docker/build.sh            # ROM + BPS: out/pokeemerald_modern.gba and out/EliteRedux.bps
docker/build.sh rom        # just the ROM
docker/build.sh --clean    # rebuild the ROM from scratch instead of incrementally
```

The outputs land in `out/` and belong to you, not root. Nothing else in your checkout is touched.

## Working on the code

```sh
docker/build.sh headers        # run the Kotlin codegen and drop include/generated + asm/generated in your tree
docker/build.sh make           # run `make` on your checkout inside the image
docker/build.sh make clean     # any make target works
docker/build.sh shell          # a shell in the image with your checkout at /src
```

`headers` is handy if you have an ARM toolchain but no Kotlin, or just want the generated
headers for your editor. `make` and `shell` build directly in your checkout, exactly like a
local build, so the outputs stay between runs. If you built the helper tools on your own
machine before, run `docker/build.sh make clean-tools` once so they get rebuilt for the container.

## How the caching works

The Dockerfile is split so each stage only reruns when its own inputs change:

| Stage | Reruns when |
|---|---|
| downloads, `builder` | a pinned version changes |
| `vanilla` (pokeemerald + flips, for the BPS) | never, in practice |
| `codegen-jars` (compiles the Kotlin codegen) | `tools/codegen` or a `.proto` schema changes |
| `codegen` (generated headers) | a `.textproto` changes; Kotlin is not recompiled |
| `tools` (gbagfx, preproc, ...) | a helper tool's source changes |
| `rom` | any source changes |

The `rom` stage keeps its work tree in a BuildKit cache mount, so object files and converted
graphics carry over between builds. Sources are synced into it by content, not timestamp, so
only what changed is rebuilt, even when you switch branches or build from several checkouts.
`--clean` empties it. The build after a clean one recompiles about two thirds of the C files once
more: the ROM Makefile only notices some headers it generates on its second run. A plain local
`make` does the same.
To drop it completely, run `docker builder prune --filter type=exec.cachemount` (Docker) or
`podman system prune --build` (Podman).

`.dockerignore` keeps your own build outputs (objects, converted graphics, host-built tools,
generated headers) out of the build, so they can't leak into the container.

## Options

| Variable | Meaning |
|---|---|
| `CONTAINER_ENGINE` | `docker` or `podman` (default: Docker if installed) |
| `JOBS` | parallel make jobs (default: all CPUs) |
| `ER_BUILD_FLAGS` | extra `build` flags, e.g. `--network host` |
| `ER_RUN_FLAGS` | extra `run` flags for `make`/`shell` |

CI runs the same build in `.github/workflows/docker.yml` and uploads the ROM and BPS.
