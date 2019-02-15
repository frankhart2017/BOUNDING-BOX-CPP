FROM bbox:latest

WORKDIR /usr/src/bounding_box/
COPY . .

WORKDIR /usr/src/bounding_box/
RUN cmake .
RUN make
CMD ["./bounding_box"]
