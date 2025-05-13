res/v0: src/common.c src/v1.c src/common.h
	@mkdir -p res/
	gcc -o $@ $^ -g -lm

res/v1: src/common.c src/v1.c src/common.h
	@mkdir -p res/
	gcc -o $@ $^ -O3 -g -lm

res/callgrind-v1.data: res/v1
	valgrind --tool=callgrind --dump-instr=yes --callgrind-out-file=res/callgrind-v1.data ./res/v1 -p data/tolkien.txt

res/v2: src/common.c src/v2.c src/common.h
	@mkdir -p res/
	gcc -o $@ $^ -O3 -g -mavx2 -lm

res/callgrind-v2.data: res/v2
	valgrind --tool=callgrind --dump-instr=yes --callgrind-out-file=res/callgrind-v2.data ./res/v2 -p data/tolkien.txt

res/v3: src/common.c src/v3.c src/v3-crc32.s src/common.h
	@mkdir -p res/
	gcc -o $@ $^ -O3 -g -mavx2 -lm

res/callgrind-v3.data: res/v3
	valgrind --tool=callgrind --dump-instr=yes --callgrind-out-file=res/callgrind-v3.data ./res/v3 -p data/tolkien.txt

res/v4: src/common.c src/v4.c src/v3-crc32.s src/common.h
	@mkdir -p res/
	gcc -o $@ $^ -O3 -g -mavx2 -lm

res/callgrind-v4.data: res/v4
	valgrind --tool=callgrind --dump-instr=yes --callgrind-out-file=res/callgrind-v4.data ./res/v4 -p data/tolkien.txt
