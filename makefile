main: c

clean:
	rm -f a.out libs3-mirror.so /usr/lib/libs3-mirror.so

cpp: s3_mirror.cpp s3_mirror.h
	g++ -std=c++17 -fpic -laws-cpp-sdk-core -laws-cpp-sdk-s3 s3_mirror.cpp -shared -o libs3-mirror.so && cp libs3-mirror.so /usr/lib/ 

c: cpp s3_mirror.c s3_mirror.h
	gcc s3_mirror.c -ls3-mirror -laws-cpp-sdk-s3 -laws-cpp-sdk-core -D_FILE_OFFSET_BITS=64 `pkg-config fuse3 --cflags --libs` -Wall -o s3m