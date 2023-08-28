FROM ubuntu:20.04
# crashoveride95 Modern N64SDK Setup
ENV N64_LIBGCCDIR=/opt/crashsdk/lib/gcc/mips64-elf/12.2.0
ENV PATH=/opt/crashsdk/bin:$PATH
ENV ROOT=/etc/n64
RUN echo "deb [trusted=yes] https://crashoveride95.github.io/apt/ ./" | tee /etc/apt/sources.list.d/n64sdk.list
RUN apt-get -y -o "Acquire::https::Verify-Peer=false" update && apt-get -y -o "Acquire::https::Verify-Peer=false" install binutils-mips-n64 gcc-mips-n64 newlib-mips-n64 n64sdk makemask libleo libhvq libhvqm libmus libnaudio libnustd libnusys git build-essential gcc-mips-linux-gnu
RUN cd ~ && git clone https://github.com/devwizard64/libcart.git && cd libcart && make && cp ./lib/libcart.a /usr/lib/n64/libcart_ultra.a && cp ./include/cart.h /usr/include/n64/
