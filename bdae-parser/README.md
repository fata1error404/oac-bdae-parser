# BDAE Opener

g++ -B/usr/lib/x86_64-linux-gnu test.cpp resFile.cpp libio.a -o test


First pass -dealing with raw offsets that can point anywhere. You're dealing with raw offsets coming from the file — these offsets might point anywhere within a removable buffer.

cpp
Copy
Edit


Does this inner pointer exactly match the file offset of any removable chunk?
Offset is expected to point to chunk start

(convert offset[i] from a relative offset into a direct pointer)



1. Утечки памяти (valgrind)
2. Аннотировать resFile.h и мб главный файл
3. README
4. Makefile