CFLAGS = -g -Wall -Wextra

main: .FORCE
	@ gcc ${CFLAGS} -o test_ht \
		unity/unity.c test_ht.c ht.c

.FORCE:
