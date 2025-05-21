#include "json.h"
#include <stdio.h>
#include <stdlib.h>

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

static JsonObject* parse_object(const char* path, FILE* file, int* line_num)
{
    // find next {
    char c = 0;
    while (c != '{' && c != EOF)
        c = fgetch(file, line_num);
    if (c == EOF) {
        print_error(path, line_num, "Missing '{'");
        return NULL;
    }
    return NULL;
}

JsonObject* json_read(const char* path)
{
    printf("reading %s\n", path);
    FILE* file = fopen(path, "r");
    if (file == NULL) {
        printf("Error opening json file: %s\n", path);
        return NULL;
    }
    int line_num = 0;
    JsonObject* object = parse_object(path, file, &line_num);
    if (fclose(file) == EOF) {
        printf("Error reading json file: %s\n", path);
        json_destroy(object);
        return NULL;
    }
    return object;
}

int json_key_exists(const JsonObject* object, const char* key)
{
    return 0;
}

JsonType json_get_type(const JsonObject* object)
{
    return JTYPE_OBJECT;
}

const JsonObject* json_get(const JsonObject* object, const char* key)
{
    return NULL;
}

const JsonArray* json_get_array(const JsonObject* object, const char* key)
{
    return NULL;
}

const char* json_get_string(const JsonObject* object, const char* key)
{
    return NULL;
}

int json_get_int(const JsonObject* object, const char* key)
{
    return 0;
}

float json_get_float(const JsonObject* object, const char* key)
{
    return 0;
}

void json_destroy(JsonObject* object)
{
}

void main(void)
{
    JsonObject* obj;
    obj = json_read("src/util/json_tests/missing_file.json");
    json_destroy(obj);
    obj = json_read("src/util/json_tests/missing_opening_brackets.json");
    json_destroy(obj);
}
