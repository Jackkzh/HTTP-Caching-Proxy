FROM ubuntu:18.04
RUN mkdir /var/log/erss

RUN apt-get update
RUN apt-get -y install libboost-all-dev
RUN apt-get -y install aptitude
RUN aptitude search boost

ADD . /var/log/erss
WORKDIR /var/log/erss

RUN chmod +x run.sh
ENTRYPOINT ["./run.sh"]
