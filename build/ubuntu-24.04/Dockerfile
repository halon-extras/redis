FROM --platform=linux/amd64 ubuntu:24.04
LABEL org.opencontainers.image.authors="support@halon.io"

ARG HALON_REPO_USER
ARG HALON_REPO_PASS

RUN apt-get update

RUN apt-get install -y wget gnupg
RUN apt-get install -y apt-transport-https

RUN wget -qO - https://raw.githubusercontent.com/halon/pgp-keys/master/pgp-keys/7F0A73B5.gpg | tee /usr/share/keyrings/halon-archive-keyring.gpg > /dev/null
RUN echo "deb [arch=`dpkg --print-architecture` signed-by=/usr/share/keyrings/halon-archive-keyring.gpg] https://repo.halon.io/ noble stable" >> /etc/apt/sources.list.d/halon.list
RUN echo "machine repo.halon.io login ${HALON_REPO_USER} password ${HALON_REPO_PASS}" >> /etc/apt/auth.conf
RUN apt-get update && apt-get install -y halon=6.3.0

RUN apt-get install -y build-essential cmake git file libssl-dev curl

RUN mkdir /tmp/halon
RUN cd /tmp/halon \
    && curl -O -L https://github.com/redis/hiredis/archive/refs/tags/v1.1.0.tar.gz \
    && tar -zxvf v1.1.0.tar.gz && cd hiredis-1.1.0 \
    && mkdir -p /hiredis && cp COPYING /hiredis/LICENSE \
    && mkdir build && cd build \
    && cmake -DENABLE_SSL=ON .. \
    && make && make install
RUN cd /tmp/halon \
    && curl -O -L https://github.com/sewenew/redis-plus-plus/archive/refs/tags/1.3.9.tar.gz \
    && tar -zxvf 1.3.9.tar.gz && cd redis-plus-plus-1.3.9 \
    && mkdir -p /redis-plus-plus && cp LICENSE /redis-plus-plus/LICENSE \
    && mkdir build && cd build \
    && cmake -DREDIS_PLUS_PLUS_USE_TLS=ON .. \
    && make && make install
RUN rm -rf /tmp/halon

RUN echo -n "UBU2404" > /OSRELEASE.txt

COPY build.sh /build.sh
CMD ["/build.sh"]
