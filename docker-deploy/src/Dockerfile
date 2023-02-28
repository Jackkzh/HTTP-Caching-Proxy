FROM ubuntu:18.04

RUN mkdir /var/log/erss

RUN apt-get update
RUN apt-get -y install libboost-all-dev

ADD . /var/log/erss

WORKDIR /var/log/erss

RUN make clean && \
    make

CMD ["./main"]
