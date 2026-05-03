FROM debian:13

RUN apt-get update 
RUN apt-get -y install git vim wget build-essential esptool screen zlib1g-dev

RUN wget https://www.python.org/ftp/python/2.7.13/Python-2.7.13.tgz \
 && tar xzf Python-2.7.13.tgz \
 && cd Python-2.7.13 \
 && ./configure --prefix=/usr/local/python2.7 \
 && make -j$(nproc) \
 && make install

RUN /usr/local/python2.7/bin/python2.7 -c "import zlib; print(zlib.ZLIB_VERSION)"
RUN echo '#!/bin/sh\nexec /usr/local/python2.7/bin/python2.7 "$@"' > /usr/bin/python \
 && chmod +x /usr/bin/python

RUN mkdir /hdd2
RUN if uname -m | grep x86_64; then \
      cd /hdd2 && \ 
      wget https://www.acsoftware.pl/files/Espressif_x64-docker.tgz && \
      tar zxvf Espressif_x64-docker.tgz && \
      rm Espressif_x64-docker.tgz && \
      cd / && \ 
      wget https://www.acsoftware.pl/files/supla-esp-signtool.tgz && \
      tar zxvf supla-esp-signtool.tgz && \
      cp -r supla-esp-signtool/* / && \
      rm -r supla-esp-signtool*; \
    else \
      cd /hdd2 && \
      wget https://www.acsoftware.pl/files/Espressif_x86-docker.tgz && \
      tar zxvf Espressif_x86-docker.tgz && \
      rm Espressif_x86-docker.tgz; \
    fi

COPY clone.sh /usr/sbin/
RUN chmod +x /usr/sbin/clone.sh

WORKDIR /CProjects/supla-espressif-esp/src
