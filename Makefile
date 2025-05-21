all:
	gcc json.c -D ASSERTS -Wall -Wextra -Werror -Wfatal-errors 

no-assert:
	gcc json.c -Wall -Wextra -Werror -Wfatal-errors
