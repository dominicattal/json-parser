#include "json.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#define UNUSED(x) (void)(x)

// https://www.json.org/json-en.html

typedef struct JsonMember {
    const char* key;
    JsonObject* value;
} JsonMember;

typedef struct JsonMembers {
    JsonMember* members;
} JsonMembers;

typedef struct JsonObject {
    JsonType type;
    union {
        const JsonMembers* val_members;
        const JsonArray* val_array;
        const char* val_string;
        const int val_int;
        const float val_float;
    };
} JsonObject;

static void object_push_member(JsonObject* object, JsonMember* member)
{
    #ifdef ASSERTS
    assert(object->type == JTYPE_OBJECT);
    #endif
    UNUSED(object);
    UNUSED(member);
}

static void print_error(const char* path, const int* line_num, const char* message)
{
    printf("Error parsing json file %s [%d]: %s\n", path, *line_num, message);
}

static char fgetch(FILE* file, int* line_num)
{
    char c = fgetc(file);
    if (c == '\n')
        (*line_num)++;
    return c;
}

static char get_next_nonspace(FILE* file, int* line_num)
{
    char c;
    do { c = fgetch(file, line_num);
    } while (c != EOF && isspace(c));
    return c;
}

static JsonMember* parse_member(const char* path, FILE* file, int* line_num, int* error_count)
{
    UNUSED(path);
    UNUSED(file);
    UNUSED(line_num);
    UNUSED(error_count);
    return NULL;
}

static JsonObject* parse_object(const char* path, FILE* file, int* line_num, int* error_count)
{
    char c;

    // find first '{'
    c = get_next_nonspace(file, line_num);
    if (c == EOF) {
        print_error(path, line_num, "Expected '{'");
        return NULL;
    }
    if (c != '{') {
        if (c == '}')
            print_error(path, line_num, "Missing '{'");
        else
            print_error(path, line_num, "Unexpected character before '{'");
        return NULL;
    }

    // create object to return
    JsonObject* object = malloc(sizeof(JsonObject));
    JsonMember* member;
    
    // keep pushing key value pairs until } is found
    // intentionally designed for null-terminated array
    do {
        member = parse_member(path, file, line_num, error_count);
        object_push_member(object, member);
    } while (member != NULL);

    // find matching closing '}'
    c = get_next_nonspace(file, line_num);

    if (c != '}') {
        print_error(path, line_num, "Expected '}'");
        json_destroy(object);
        return NULL;
    }

    return object;
}

JsonObject* json_read(const char* path)
{
    printf("Parsing %s\n\n", path);
    FILE* file = fopen(path, "r");
    if (file == NULL) {
        printf("Error opening json file: %s\n", path);
        printf("\nParsing failed\n");
        return NULL;
    }
    int line_num = 1;
    int error_count = 0;
    JsonObject* object = parse_object(path, file, &line_num, &error_count);
    if (object != NULL && get_next_nonspace(file, &line_num) != EOF) {
        print_error(path, &line_num, "Excess characters");
        json_destroy(object);
        return NULL;
    }
    if (fclose(file) == EOF) {
        printf("Error reading json file: %s\n", path);
        json_destroy(object);
        return NULL;
    }
    if (object == NULL) {
        printf("\nParsing failed\n");
        json_destroy(object);
        return NULL;
    }
    printf("\nParsing success\n");
    return object;
}

int json_key_exists(const JsonObject* object, const char* key)
{
    UNUSED(object);
    UNUSED(key);
    return 0;
}

JsonType json_get_type(const JsonObject* object)
{
    UNUSED(object);
    return JTYPE_OBJECT;
}

const JsonObject* json_get(const JsonObject* object, const char* key)
{
    UNUSED(object);
    UNUSED(key);
    return NULL;
}

const JsonArray* json_get_array(const JsonObject* object, const char* key)
{
    UNUSED(object);
    UNUSED(key);
    return NULL;
}

const char* json_get_string(const JsonObject* object, const char* key)
{
    UNUSED(object);
    UNUSED(key);
    return NULL;
}

int json_get_int(const JsonObject* object, const char* key)
{
    UNUSED(object);
    UNUSED(key);
    return 0;
}

float json_get_float(const JsonObject* object, const char* key)
{
    UNUSED(object);
    UNUSED(key);
    return 0;
}

void json_destroy(JsonObject* object)
{
    UNUSED(object);
}

const char* tests[] = {
    "missing_file",
    "missing_opening_brackets",
    "unrecognized_char",
    "missing_closing_brackets",
    "characters_after_root",
    "test1"
};

int main(void)
{
    JsonObject* obj;
    char str[256];
    for (int i = 0; i < (int)(sizeof(tests)/sizeof(char*)); i++) {
        sprintf(str, "tests/%s.json", tests[i]);
        obj = json_read(str);
        json_destroy(obj);
        puts("----------------------------");
    }
    return 0;
}
