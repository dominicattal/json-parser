# JSON Parser

This program provides a thread-safe API for reading and accessing JSON files in C. Simply add the header file `json.h` and source file `json.c` to your project to use.
You can replace the allocator used by redefining the `json_malloc`, `json_realloc`, and `json_free` macros in `json.c`. You can also define your
own error messages by redefining the `print_error` macro, but the errors are not verbose and are turned off by default.

Key-value pairs in the json objects are stored in alphabetical order. This allows log-linear lookup times and sorted iteration; however, this way of accessing values,
by using a binary search on the keys, makes duplicate keys difficult to use. While this program does not prevent duplicate keys in the json (it is valid json
after all), it cannot reliably access the duplicate keys.
