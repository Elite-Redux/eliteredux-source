# Building with Docker or Podman

`docker/build.sh` builds Elite Redux inside a container. You don't need an ARM toolchain,
Java, Kotlin, Poryscript or protoc on your machine: the container has all of them, at the
same versions CI uses.

## What you need

- Docker 23 or newer, or Podman 4 or newer
- The er-config submodule (the game data in `proto/`):

  ```sh
  git submodule update --init
  ```

The container is `linux/amd64`. It works on Apple Silicon, but runs emulated, so it's slower there.

## Build the ROM and patch

```sh
docker/build.sh            # out/EliteRedux.gba and out/EliteRedux.bps
docker/build.sh rom        # only out/EliteRedux.gba
```

Everything ends up in `out/`. Nothing else in your checkout is changed. `EliteRedux.gba` is the
same file a normal `make` produces as `pokeemerald_modern.gba` (that name comes from pokeemerald's
Makefile), just renamed on the way out. `docker/build.sh make` builds in your checkout, so it
keeps the Makefile's name.

The first build downloads and sets up the tools, which takes a while. After that, a build
only redoes what your changes affect:

| You changed | What gets rebuilt |
|---|---|
| nothing | nothing, the previous result is reused |
| a C file | that file, then the ROM is linked again |
| a `.textproto` in `proto/` | the generated headers, then the C files using them |
| the Kotlin code in `tools/codegen` | the codegen too |

If a build ever looks wrong, `docker/build.sh --clean` rebuilds the ROM from scratch.
The build right after a clean one recompiles about two thirds of the C files once more,
because some headers are only generated during the first build. A normal `make` does the same.

## Other commands

```sh
docker/build.sh headers      # generate include/generated and asm/generated into your checkout
docker/build.sh make         # run make on your checkout, inside the container
docker/build.sh make clean   # any make target works
docker/build.sh shell        # open a shell in the container, with your checkout in /src
```

- `headers` is for when you have an ARM toolchain but no Kotlin: generate the headers in the
  container, then build with your own `make`. It's also handy for editors that need the
  generated headers.
- `make` and `shell` work directly in your checkout, like a local build. If you built the helper
  tools on your own machine before, run `docker/build.sh make clean-tools` once first.

## Options

| Variable | What it does |
|---|---|
| `CONTAINER_ENGINE` | `docker` or `podman` (default: Docker if it's installed) |
| `JOBS` | how many files to build in parallel (default: all CPU cores) |
| `ER_BUILD_FLAGS` | extra flags for `docker build` / `podman build`, e.g. `--network host` |
| `ER_RUN_FLAGS` | extra flags for `docker run` / `podman run`, for `make` and `shell` |

## Freeing disk space

The tools and the kept build files take a few GB. To remove them:

```sh
docker builder prune           # Docker
podman system prune --build    # Podman
```

## CI

`.github/workflows/docker.yml` runs the same build on every pull request to check that the ROM
and patch still build. It doesn't upload them. It keeps the tools and other unchanged parts
between runs with the GitHub Actions cache, so most runs only redo the codegen and the ROM.
