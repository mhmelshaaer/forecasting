
### BUILD stage
FROM frolvlad/alpine-gcc:latest as build

RUN apk update && apk add gcc g++ make cmake linux-headers

ARG SRC_DIR=/var/src/forecasting
ARG SERVER_HOST_NAME

RUN mkdir -p $SRC_DIR
WORKDIR $SRC_DIR

COPY . "$SRC_DIR/"

RUN cmake -S. -Bbuild -DSERVER_HOST_NAME=${SERVER_HOST_NAME} && cmake --build build


### RUNTIME stage
FROM frolvlad/alpine-gcc:latest

ARG SRC_DIR=/var/src/forecasting

RUN mkdir -p $SRC_DIR
WORKDIR $SRC_DIR

COPY docker-entrypoint.sh $SRC_DIR

COPY --from=build $SRC_DIR/build $SRC_DIR

RUN chmod +x docker-entrypoint.sh

ENTRYPOINT [ "./docker-entrypoint.sh" ]