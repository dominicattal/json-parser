all:
	gcc json.c -D JSON_ASSERTS -Wall -Wextra -Werror -Wfatal-errors 

no-assert:
	gcc json.c -Wall -Wextra -Werror -Wfatal-errors
