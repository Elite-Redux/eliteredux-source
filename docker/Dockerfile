# syntax=docker/dockerfile:1
#
# Containerised Elite Redux build: no local ARM toolchain, JDK or Kotlin needed.
# Use docker/build.sh rather than calling this directly; see docs/docker-build.md.
#
# Stages, from least to most often rebuilt:
#   fetch-*        pinned, checksummed downloads (toolchain, kotlinc, poryscript, protoc, jars)
#   builder        everything needed to run `make`; also the image behind `docker/build.sh make`
#   vanilla        pokeemerald.gba built with agbcc, plus flips (only needed for the BPS)
#   codegen-jars   Kotlin codegen compiled from tools/codegen + proto/*.proto
#   codegen        generated headers; reruns on textproto edits without recompiling Kotlin
#   tools          host helper tools (gbagfx, preproc, ...)
#   rom            the ROM, built incrementally in a cache mount
#   bps            EliteRedux.bps
#   *-artifact     scratch stages for `--output type=local`

ARG DEBIAN_IMAGE=debian:bookworm-slim

FROM ${DEBIAN_IMAGE} AS fetch
RUN apt-get update \
 && apt-get install -y --no-install-recommends ca-certificates curl unzip xz-utils \
 && rm -rf /var/lib/apt/lists/*
WORKDIR /dl

FROM fetch AS fetch-arm-toolchain
# Ubuntu/Debian's gcc-arm-none-eabi 12/13 can't do -std=gnu23, so use Arm's own build.
ARG ARM_TOOLCHAIN_VERSION=14.2.rel1
ARG ARM_TOOLCHAIN_SHA256=62a63b981fe391a9cbad7ef51b17e49aeaa3e7b0d029b36ca1e9c3b2a9b78823
RUN curl -fsSL -o toolchain.tar.xz "https://developer.arm.com/-/media/Files/downloads/gnu/${ARM_TOOLCHAIN_VERSION}/binrel/arm-gnu-toolchain-${ARM_TOOLCHAIN_VERSION}-x86_64-arm-none-eabi.tar.xz" \
 && echo "${ARM_TOOLCHAIN_SHA256}  toolchain.tar.xz" | sha256sum -c - \
 && mkdir -p /opt/arm-gnu-toolchain \
 && tar xJf toolchain.tar.xz -C /opt/arm-gnu-toolchain --strip-components=1 \
 && rm toolchain.tar.xz
# The toolchain ships libraries for ~40 Arm CPU/FPU variants (~600 MB). The GBA only links
# the one the ROM Makefile's flags select (thumb/nofp), so drop the others, plus the parts
# the build never runs: Fortran, LTO, gdb, GCC plugins and docs.
RUN set -eu; t=/opt/arm-gnu-toolchain; \
    keep=$("$t/bin/arm-none-eabi-gcc" -mthumb -mthumb-interwork -mabi=apcs-gnu -march=armv4t -print-multi-directory); \
    for base in "$t/arm-none-eabi/lib" "$t"/lib/gcc/arm-none-eabi/*; do \
        for dir in "$base"/thumb/* "$base"/arm/*; do \
            [ "${dir#"$base"/}" = "${keep}" ] || rm -rf "${dir}"; \
        done; \
    done; \
    rm -rf "$t"/bin/*gfortran* "$t"/bin/*gdb* "$t"/bin/*lto-dump* "$t"/share \
        "$t"/libexec/gcc/arm-none-eabi/*/f951 "$t"/libexec/gcc/arm-none-eabi/*/lto1 \
        "$t"/lib/gcc/arm-none-eabi/*/plugin "$t"/arm-none-eabi/lib/libgfortran*; \
    test -f "$t/arm-none-eabi/lib/${keep}/libc.a"

FROM fetch AS fetch-kotlin
ARG KOTLIN_VERSION=2.4.21
ARG KOTLIN_SHA256=7cc140e76daf416a0424a5557f99afd7ca89ebd463ea1101261e3e01c33e75cd
RUN curl -fsSL -o kotlin.zip "https://github.com/JetBrains/kotlin/releases/download/v${KOTLIN_VERSION}/kotlin-compiler-${KOTLIN_VERSION}.zip" \
 && echo "${KOTLIN_SHA256}  kotlin.zip" | sha256sum -c - \
 && unzip -q kotlin.zip -d /opt \
 && rm kotlin.zip

FROM fetch AS fetch-poryscript
# Same version as CI; 3.6.1 is known to be broken for this repo.
ARG PORYSCRIPT_VERSION=3.5.2
ARG PORYSCRIPT_SHA256=cda085d4662b630a6645f33cf8773602821b35977ccaafbb02513b36008cdf5c
RUN curl -fsSL -o poryscript.zip "https://github.com/huderlem/poryscript/releases/download/${PORYSCRIPT_VERSION}/poryscript-linux.zip" \
 && echo "${PORYSCRIPT_SHA256}  poryscript.zip" | sha256sum -c - \
 && unzip -q poryscript.zip \
 && mv poryscript-linux /opt/poryscript \
 && chmod 0755 /opt/poryscript/poryscript

FROM fetch AS fetch-codegen-deps
# The exact files tools/codegen/makefile would otherwise download on every fresh checkout.
ARG PROTOC_VERSION=29.2
ARG PROTOC_SHA256=52e9e7ece55c7e30e7e8bbd254b4b21b408a5309bca826763c7124b696a132e9
ARG PROTOBUF_JAVA_VERSION=4.29.2
ARG PROTOBUF_JAVA_SHA256=7441a6394dca477b3f8b7c86f17bef035f705a8aaa11f53065d380300158177c
ARG PROTOBUF_KOTLIN_SHA256=2fbdd6977f446b1030fbdc2eb083d8eb54b41cdc854bfba996245e4b68c9152e
RUN mkdir -p /opt/codegen-deps \
 && curl -fsSL -o protoc.zip "https://github.com/protocolbuffers/protobuf/releases/download/v${PROTOC_VERSION}/protoc-${PROTOC_VERSION}-linux-x86_64.zip" \
 && echo "${PROTOC_SHA256}  protoc.zip" | sha256sum -c - \
 && unzip -p protoc.zip bin/protoc > /opt/codegen-deps/protoc \
 && chmod 0755 /opt/codegen-deps/protoc \
 && cp protoc.zip /opt/codegen-deps/protoc.zip \
 && curl -fsSL -o /opt/codegen-deps/protobuf-java.jar "https://repo1.maven.org/maven2/com/google/protobuf/protobuf-java/${PROTOBUF_JAVA_VERSION}/protobuf-java-${PROTOBUF_JAVA_VERSION}.jar" \
 && curl -fsSL -o /opt/codegen-deps/protobuf-kotlin.jar "https://repo1.maven.org/maven2/com/google/protobuf/protobuf-kotlin/${PROTOBUF_JAVA_VERSION}/protobuf-kotlin-${PROTOBUF_JAVA_VERSION}.jar" \
 && echo "${PROTOBUF_JAVA_SHA256}  /opt/codegen-deps/protobuf-java.jar" | sha256sum -c - \
 && echo "${PROTOBUF_KOTLIN_SHA256}  /opt/codegen-deps/protobuf-kotlin.jar" | sha256sum -c -

FROM ${DEBIAN_IMAGE} AS builder
RUN --mount=type=cache,id=eliteredux-apt-cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,id=eliteredux-apt-lists,target=/var/lib/apt/lists,sharing=locked \
    rm -f /etc/apt/apt.conf.d/docker-clean \
 && apt-get update \
 && apt-get install -y --no-install-recommends \
      ca-certificates \
      g++ \
      gcc \
      libc6-dev \
      libpng-dev \
      make \
      openjdk-17-jdk-headless \
      perl \
      pkg-config \
      rsync \
      unzip \
      wget \
 && rm -rf /usr/lib/jvm/*/jmods
COPY --from=fetch-arm-toolchain /opt/arm-gnu-toolchain /opt/arm-gnu-toolchain
COPY --from=fetch-kotlin /opt/kotlinc /opt/kotlinc
COPY --from=fetch-poryscript /opt/poryscript /opt/poryscript
COPY --from=fetch-codegen-deps /opt/codegen-deps /opt/codegen-deps
# The ROM Makefile only finds arm-none-eabi-gcc through $DEVKITARM/bin.
ENV DEVKITARM=/opt/arm-gnu-toolchain \
    PATH=/opt/kotlinc/bin:/opt/arm-gnu-toolchain/bin:${PATH}
COPY docker/prepare-tree.sh /usr/local/bin/er-prepare-tree
WORKDIR /src

FROM ${DEBIAN_IMAGE} AS vanilla
# Vanilla Emerald for the BPS base, built from pinned pret sources and checked against rom.sha1.
ARG AGBCC_REF=da598c1d918402c42c0c0d7128ba14567f3175e9
ARG POKEEMERALD_REF=9a83a2bbe8e097e62c00f1dbd56849766775d7b6
ARG FLIPS_REF=ff216a75df0987047a67d7923567dc4482ce07ac
RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      binutils-arm-none-eabi build-essential ca-certificates git libpng-dev pkg-config \
 && rm -rf /var/lib/apt/lists/*
RUN git init -q /src/pokeemerald \
 && git -C /src/pokeemerald fetch -q --depth 1 https://github.com/pret/pokeemerald.git "${POKEEMERALD_REF}" \
 && git -C /src/pokeemerald checkout -q --detach FETCH_HEAD \
 && git init -q /src/agbcc \
 && git -C /src/agbcc fetch -q --depth 1 https://github.com/pret/agbcc.git "${AGBCC_REF}" \
 && git -C /src/agbcc checkout -q --detach FETCH_HEAD \
 && cd /src/agbcc && ./build.sh && ./install.sh /src/pokeemerald \
 && make -C /src/pokeemerald -j"$(nproc)" \
 && cd /src/pokeemerald && sha1sum -c rom.sha1 \
 && mkdir -p /out && cp pokeemerald.gba /out/ \
 && rm -rf /src/agbcc /src/pokeemerald
RUN git init -q /src/flips \
 && git -C /src/flips fetch -q --depth 1 https://github.com/Alcaro/Flips.git "${FLIPS_REF}" \
 && git -C /src/flips checkout -q --detach FETCH_HEAD \
 && make -C /src/flips -j"$(nproc)" TARGET=cli CFLAGS=-O3 \
 && cp /src/flips/flips /out/flips \
 && rm -rf /src/flips

FROM builder AS codegen-jars
# Only the Kotlin sources and .proto schemas: data (.textproto) edits don't recompile Kotlin.
COPY tools/codegen/makefile tools/codegen/
COPY tools/codegen/src/ tools/codegen/src/
COPY proto/*.proto proto/
COPY proto/google/ proto/google/
RUN cp -p /opt/codegen-deps/* tools/codegen/ \
 && make -C tools/codegen codegen.jar codegenkt.jar preproc.jar

FROM codegen-jars AS codegen
# The generators also read the font width tables (text wrapping).
COPY graphics/fonts/*_latin_widths.inc graphics/fonts/
COPY proto/ proto/
RUN make -C tools/codegen -j"$(nproc)"

FROM builder AS tools
# Host helper tools (gbagfx, preproc, ...). tools/codegen is left out so codegen edits
# don't rebuild them; it is built by the codegen stages instead. It can't go in
# .dockerignore because the codegen stages need it. Docker matches --exclude against the
# copied directory and Podman against the build context, hence both patterns.
COPY make_tools.mk ./
COPY --exclude=codegen --exclude=tools/codegen tools/ tools/
RUN make -f make_tools.mk -j"$(nproc)" TOOLDIRS="$(ls -d tools/*/ | sed 's:/$::' | grep -v '^tools/\(codegen\|agbcc\|binutils\|poryscript\)$' | tr '\n' ' ')"

FROM builder AS rom
# Set INCREMENTAL=0 (docker/build.sh --clean) to throw away the cached work tree.
ARG INCREMENTAL=1
ARG JOBS=
COPY . .
COPY --from=tools /src/tools/ tools/
COPY --from=codegen /src/include/generated/ include/generated/
COPY --from=codegen /src/asm/generated/ asm/generated/
RUN er-prepare-tree
RUN --mount=type=cache,id=eliteredux-rom-work,target=/work,sharing=locked \
    INCREMENTAL="${INCREMENTAL}" JOBS="${JOBS}" sh docker/build-rom.sh /src /work /out

FROM builder AS bps
COPY --from=vanilla /out/ /opt/vanilla/
COPY --from=rom /out/ /out/
RUN /opt/vanilla/flips --create /opt/vanilla/pokeemerald.gba /out/pokeemerald_modern.gba /out/EliteRedux.bps

FROM scratch AS headers-artifact
COPY --from=codegen /src/include/generated/ /include/generated/
COPY --from=codegen /src/asm/generated/ /asm/generated/

FROM scratch AS rom-artifact
COPY --from=rom /out/ /

FROM scratch AS bps-artifact
COPY --from=bps /out/ /
