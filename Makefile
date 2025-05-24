all:
	gcc json.c test.c -D JSON_ASSERTS -D JSON_VERBOSE -Wall -Wextra -Werror -Wfatal-errors 

no-assert:
	gcc json.c -Wall -Wextra -Werror -Wfatal-errors
