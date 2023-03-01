FROM ubuntu:18.04
RUN mkdir /var/log/erss 
RUN mkdir /app

RUN apt-get update
RUN apt-get -y install libboost-all-dev
# RUN apt-get -y install aptitude
# RUN aptitude search boost  

ADD . /app
WORKDIR /app

RUN make clean
RUN make

RUN chmod +x run.sh
ENTRYPOINT ["./run.sh"]



