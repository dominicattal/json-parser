all:
	gcc -std=c99 json.c test.c -g3 -Wall -Wextra -Werror -Wfatal-errors -Wpedantic -Wno-unused-parameter -Wdeclaration-after-statement
