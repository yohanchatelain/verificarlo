#
# Dockerfile for Verificarlo (github.com/yohanchatelain/verificarlo)
# This image includes support for Fortran and uses llvm-7 and gcc-7
#

FROM ubuntu:19.10
MAINTAINER Yohan Chatelain <yohan.chatelain@uvsq.fr>

# Retrieve dependencies
RUN apt-get -y update && apt-get -y --no-install-recommends install tzdata 
RUN apt-get -y --no-install-recommends install \
    bash ca-certificates make git \
    autogen dh-autoreconf autoconf autogen automake autotools-dev dh-autoreconf libedit-dev libtool libz-dev binutils \
    clang-7 llvm-7 llvm-7-dev gcc-7 g++-7 \
    python3 python3-distutils python3-dev python3-numpy python3-matplotlib libmpfr-dev \
    flang libgfortran-9-dev cython3

# ENV LIBRARY_PATH /usr/lib:$LIBRARY_PATH
ENV LIBRARY_PATH /usr/lib/gcc/x86_64-linux-gnu/7:/usr/lib/llvm-7/lib:$LIBRARY_PATH

# Download and configure verificarlo from git master
RUN git clone  --branch ubuntu-19.10 https://github.com/yohanchatelain/verificarlo.git  && \
    cd verificarlo && \
    ./autogen.sh && \
    ./configure --with-llvm=$(llvm-config-7 --prefix) --with-flang CC=gcc-7 CXX=g++-7 || cat config.log 

# Build and test verificarlo

ENV LD_LIBRARY_PATH /usr/local/lib:$LD_LIBRARY_PATH
ENV PATH /usr/local/bin:$PATH
ENV PYTHONPATH /usr/local/lib/python3.7/site-packages/:$PYTHONPATH
RUN cd verificarlo && make && make install && make installcheck || for i in $(find tests/ -maxdepth 1 -type d -name 'test_*' | sort ); do echo "************** TEST $i"; cat $i/test.log; done;


# Setup working directory
VOLUME /workdir
WORKDIR /workdir


