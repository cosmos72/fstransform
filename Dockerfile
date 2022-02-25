FROM ubuntu AS build

RUN apt-get update && apt-get install build-essential automake -y

COPY . /app/src
WORKDIR /app/src

RUN ./configure
RUN make


FROM ubuntu

RUN apt-get update && \
    apt-get install xfsprogs reiserfsprogs jfsutils -y

COPY --from=build \
    /app/src/fsmove/build/fsmove \
    /app/src/fsmount_kernel/build/fsmount_kernel \
    /app/src/fsremap/build/fsremap \
    /app/src/fstransform/build/fstransform \
    /usr/bin/


RUN mkdir -p /app/work

VOLUME /var/tmp/fstransform
VOLUME /app/work

WORKDIR /app/work

ENTRYPOINT fstransform
