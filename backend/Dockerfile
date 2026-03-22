FROM ubuntu:24.04

RUN apt-get update && apt-get install -y g++ && rm -rf /var/lib/apt/lists/*

WORKDIR /backend
COPY . .

RUN g++ -O2 -std=c++17 -Isrc/includes -o server src/*.cpp src/graph/*.cpp

CMD ["./server"]
