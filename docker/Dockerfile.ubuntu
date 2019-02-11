FROM ubuntu:latest

EXPOSE 9090

RUN apt-get update -qq && apt-get install -y -qq --no-install-recommends \
       build-essential cmake libboost-all-dev

RUN mkdir -p /opt/confluo
COPY . /opt/confluo

WORKDIR /opt/confluo
RUN mkdir build \
    && cd build \
    && cmake -DBUILD_TESTS=OFF -DWITH_PY_CLIENT=OFF -DWITH_JAVA_CLIENT=OFF .. \
    && make -j8 install

RUN mkdir -p /var/db
VOLUME /var/db

ENTRYPOINT ["confluod"]
CMD ["--address=0.0.0.0", "--port=9090", "--data-path=/var/db"]
