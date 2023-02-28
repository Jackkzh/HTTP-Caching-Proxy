FROM ubuntu:18.04

RUN mkdir /var/log/erss

RUN apt-get update && \
    apt-get -y install g++ make libboost-all-dev

ADD . /var/log/erss

WORKDIR /var/log/erss

RUN make clean && \
    make

CMD ["./main"]
