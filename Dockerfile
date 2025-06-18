FROM alpine:latest AS clangmusl
RUN apk add --no-cache clang musl-dev binutils upx

FROM clangmusl AS builder
COPY src/ /workdir
WORKDIR /workdir

RUN clang -std=c99 -march=native -flto -ffast-math -static \
          -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_POSIX_C_SOURCE=200809L \
          ghost.c frames.c -o ghost 

RUN  upx -9 ghost

FROM scratch
COPY --from=builder /workdir/ghost /ghost
WORKDIR /
ENTRYPOINT ["/ghost"]
