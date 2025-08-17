CFLAGS = -g -Wall -Wextra -Werror

main:
	@ gcc ${CFLAGS} -o test_ht \
		unity/unity.c test_ht.c ht.c
