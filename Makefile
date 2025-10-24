CFLAGS = -O3 -g -Wall -Werror -Wextra -pedantic-errors

main: .FORCE
	@ gcc ${CFLAGS} -o test_ht \
		unity/unity.c test_ht.c ht.c

.FORCE:
