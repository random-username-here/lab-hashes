PROGS="v0 v1 v2 v3 v4"

TARGETS=""
for PROG in $PROGS; do
	TARGETS+="res/$PROG "
done
make $TARGETS

echo > res/time.tsv

for RUN in $(seq 10); do
	for PROG in $PROGS; do
		NUM=$(grep -oh '[0-9]' <<<"$PROG")
		printf "Run #${RUN} of version ${NUM} - measuring..."
		TIME=$(./res/${PROG} data/tolkien.txt | grep -Poh '[0-9]+\.[0-9]+\s*(?=ms)')
		printf "\rRun #${RUN} of version ${NUM} - runtime is ${TIME} ms\n"
		printf "${NUM}\t${RUN}\t${TIME}\n" >> res/time.tsv
	done
done
