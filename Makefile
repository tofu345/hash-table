CFLAGS = -g -Wall -Wextra

main:
	@ gcc ${CFLAGS} -o test_ht \
		unity/unity.c test_ht.c ht.c
