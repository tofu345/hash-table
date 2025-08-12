FILES = test_ht.c ht.c
OUTFILE = test_ht
CFLAGS = -g -o ${OUTFILE}

demo: ${FILES} Makefile
	@ gcc ${CFLAGS} ${FILES}
