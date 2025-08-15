FROM --platform=linux/amd64 ubuntu:20.04
LABEL org.opencontainers.image.authors="support@halon.io"

ARG HALON_REPO_USER
ARG HALON_REPO_PASS

RUN apt-get update

RUN apt-get install -y wget gnupg
RUN apt-get install -y apt-transport-https

RUN wget -qO - https://raw.githubusercontent.com/halon/changelog/master/pgp-keys/7F0A73B5.asc | apt-key add -
RUN echo "deb https://repo.halon.io/ focal stable" >> /etc/apt/sources.list.d/halon.list
RUN echo "machine repo.halon.io login ${HALON_REPO_USER} password ${HALON_REPO_PASS}" >> /etc/apt/auth.conf
RUN apt-get update && apt-get install -y halon=5.10.0

RUN apt-get install -y build-essential cmake git libssl-dev curl

RUN mkdir /tmp/halon
RUN cd /tmp/halon \
    && curl -O -L https://github.com/redis/hiredis/archive/refs/tags/v1.1.0.tar.gz \
    && echo "fe6d21741ec7f3fc9df409d921f47dfc73a4d8ff64f4ac6f1d95f951bf7f53d6 v1.1.0.tar.gz" | sha256sum -c \
    && tar -zxvf v1.1.0.tar.gz && cd hiredis-1.1.0 \
    && mkdir -p /hiredis && cp COPYING /hiredis/LICENSE \
    && mkdir build && cd build \
    && cmake -DENABLE_SSL=ON .. \
    && make && make install
RUN cd /tmp/halon \
    && curl -O -L https://github.com/sewenew/redis-plus-plus/archive/refs/tags/1.3.9.tar.gz \
    && echo "f890bee2bf81e85619e8cdd1189b9249daddf1562a89e04e2b685ba4ca37e0f8 1.3.9.tar.gz" | sha256sum -c \
    && tar -zxvf 1.3.9.tar.gz && cd redis-plus-plus-1.3.9 \
    && mkdir -p /redis-plus-plus && cp LICENSE /redis-plus-plus/LICENSE \
    && mkdir build && cd build \
    && cmake -DREDIS_PLUS_PLUS_USE_TLS=ON .. \
    && make && make install
RUN rm -rf /tmp/halon

RUN echo -n "UBU2004" > /OSRELEASE.txt

COPY build.sh /build.sh
CMD ["/build.sh"]
