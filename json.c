#include "json.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNUSED(x) (void)(x)
#ifdef JSON_ASSERTS
    #define ASSERT(x) assert(x)
#else
    #define ASSERT(x) ((void)0)
#endif

// https://www.json.org/json-en.html

typedef struct JsonMember {
    const char* key;
    JsonValue* value;
} JsonMember;

typedef struct JsonObject {
    const JsonMember** members;
} JsonObject;

typedef struct JsonValue {
    JsonType type;
    union {
        const JsonObject** val_members;
        const JsonArray* val_array;
        const char* val_string;
        const int val_int;
        const float val_float;
    };
} JsonValue;

static JsonValue* parse_value(const char* path, FILE* file, int* line_num, int* error_count);
static JsonValue* parse_value_object(const char* path, FILE* file, int* line_num, int* error_count);
static JsonValue* parse_value_array(const char* path, FILE* file, int* line_num, int* error_count);
static JsonValue* parse_value_number(const char* path, FILE* file, int* line_num, int* error_count);
static JsonValue* parse_value_string(const char* path, FILE* file, int* line_num, int* error_count);
static JsonValue* parse_value_true(const char* path, FILE* file, int* line_num, int* error_count);
static JsonValue* parse_value_false(const char* path, FILE* file, int* line_num, int* error_count);
static JsonValue* parse_value_null(const char* path, FILE* file, int* line_num, int* error_count);
static JsonObject* parse_object(const char* path, FILE* file, int* line_num, int* error_count);
static JsonMember* parse_member(const char* path, FILE* file, int* line_num, int* error_count);
static JsonMember** parse_members(const char* path, FILE* file, int* line_num, int* error_count);

static void json_members_destroy(JsonMember** members)
{
    UNUSED(members);
}

static int json_member_cmp(const void* x, const void* y)
{
    const JsonMember* m1 = x;
    const JsonMember* m2 = y;
    return strcmp(m1->key, m2->key);
}

static JsonMember** push_member(JsonMember** members, JsonMember* member, int* length)
{   
    if (members == NULL)
        members = malloc(2 * sizeof(JsonMember*));
    else
        members = realloc(members, (*length+2) * sizeof(JsonMember*));
    ASSERT(members != NULL);
    members[(*length)++] = member;
    members[(*length)] = NULL;
    return members;
}

static void print_error(const char* path, const int* line_num, const char* message)
{
    printf("Error parsing json file %s [%d]: %s\n", path, *line_num, message);
}

static char getch(FILE* file, int* line_num)
{
    char c = getc(file);
    if (c == '\n')
        (*line_num)++;
    return c;
}

static void ungetch(FILE* file, int* line_num, char c)
{
    if (c == '\n')
        (*line_num)--;
    ungetc(c, file);
}

static char get_next_nonspace(FILE* file, int* line_num)
{
    char c;
    do { c = getch(file, line_num);
    } while (c != EOF && isspace(c));
    return c;
}

static char peek_next_nonspace(FILE* file, int* line_num)
{
    char c = get_next_nonspace(file, line_num);
    ungetch(file, line_num, c);
    return c;
}

// get position in file stream of next instance of char 'ch'
// returns -2 if it reaches end of file
static long get_next_char_pos(FILE* file, int* line_num, char ch)
{
    char c;
    do { c = getch(file, line_num);
    } while (c != EOF && c != ch);
    return (c != EOF) ? ftell(file) : -2;
}

// gets next string surrounded by quotes, returns NULL if not valid
static const char* get_next_string(const char* path, FILE* file, int* line_num)
{
    char c;
    long start_pos, end_pos;

    c = get_next_nonspace(file, line_num);
    if (c != '"') {
        print_error(path, line_num, "Missing quotes");
        return NULL;
    }

    start_pos = ftell(file);
    ASSERT(start_pos != -1);
    end_pos = get_next_char_pos(file, line_num, '"');
    ASSERT(end_pos != -1);
    if (end_pos == -2) {
        print_error(path, line_num, "Expected closing quotes");
        return NULL;
    }

    fseek(file, start_pos, SEEK_SET);
    int n = end_pos - start_pos;
    char* string = malloc(n * sizeof(char));
    fgets(string, n+1, file);
    string[n-1] = '\0';
    return string;
}

static JsonValue* parse_value_object(const char* path, FILE* file, int* line_num, int* error_count)
{
    UNUSED(path);
    UNUSED(file);
    UNUSED(line_num);
    UNUSED(error_count);
    return NULL;
}

static JsonValue* parse_value_array(const char* path, FILE* file, int* line_num, int* error_count)
{
    UNUSED(path);
    UNUSED(file);
    UNUSED(line_num);
    UNUSED(error_count);
    return NULL;
}

static JsonValue* parse_value_number(const char* path, FILE* file, int* line_num, int* error_count)
{
    UNUSED(path);
    UNUSED(file);
    UNUSED(line_num);
    UNUSED(error_count);
    return NULL;
}

static JsonValue* parse_value_string(const char* path, FILE* file, int* line_num, int* error_count)
{
    UNUSED(error_count);

    const char* string = get_next_string(path, file, line_num);

    JsonValue* value = malloc(sizeof(JsonValue));
    value->type = JTYPE_STRING;
    value->val_string = string;

    return value;
}

static JsonValue* parse_value_true(const char* path, FILE* file, int* line_num, int* error_count)
{
    UNUSED(path);
    UNUSED(file);
    UNUSED(line_num);
    UNUSED(error_count);
    return NULL;
}

static JsonValue* parse_value_false(const char* path, FILE* file, int* line_num, int* error_count)
{
    UNUSED(path);
    UNUSED(file);
    UNUSED(line_num);
    UNUSED(error_count);
    return NULL;
}

static JsonValue* parse_value_null(const char* path, FILE* file, int* line_num, int* error_count)
{
    UNUSED(path);
    UNUSED(file);
    UNUSED(line_num);
    UNUSED(error_count);
    return NULL;
}

static JsonValue* parse_value(const char* path, FILE* file, int* line_num, int* error_count)
{
    char c = peek_next_nonspace(file, line_num);
    if (c == '[')
        return parse_value_array(path, file, line_num, error_count);
    if (c == '{')
        return parse_value_object(path, file, line_num, error_count);
    if (c == '-' || isdigit(c))
        return parse_value_number(path, file, line_num, error_count);
    if (c == '"')
        return parse_value_string(path, file, line_num, error_count);
    if (c == 't')
        return parse_value_true(path, file, line_num, error_count);
    if (c == 'f')
        return parse_value_false(path, file, line_num, error_count);
    if (c == 'n')
        return parse_value_null(path, file, line_num, error_count);
    print_error(path, line_num, "Invalid value");
    return NULL;
}

static JsonMember* parse_member(const char* path, FILE* file, int* line_num, int* error_count)
{
    char c;
    
    const char* key = get_next_string(path, file, line_num);
    if (key == NULL) {
        print_error(path, line_num, "Error reading key");
        return NULL;
    }

    c = get_next_nonspace(file, line_num);
    if (c != ':') {
        print_error(path, line_num, "Missing colon");
        return NULL;
    }

    JsonValue* value;
    value = parse_value(path, file, line_num, error_count);
    if (value == NULL) {
        print_error(path, line_num, "Error reading value");
        return NULL;
    }

    JsonMember* member = malloc(sizeof(JsonMember));
    ASSERT(member != NULL);
    member->key = key;
    member->value = value;

    UNUSED(path);
    UNUSED(error_count);

    return member;
}

static JsonMember** parse_members(const char* path, FILE* file, int* line_num, int* error_count)
{
    char c;
    int num_members = 0;
    JsonMember** members = NULL;
    JsonMember* member;

    member = parse_member(path, file, line_num, error_count);
    if (member == NULL) {
        print_error(path, line_num, "Error parsing member");
        return NULL;
    }

    members = push_member(members, member, &num_members);
    c = peek_next_nonspace(file, line_num);
    if (c == '}')
        return members;

    do {
        c = get_next_nonspace(file, line_num);
        if (c != ',') {
            print_error(path, line_num, "Missing comma between members");
            json_members_destroy(members);
            return NULL;
        }
        member = parse_member(path, file, line_num, error_count);
        if (member == NULL) {
            print_error(path, line_num, "Error parsing member");
            json_members_destroy(members);
            return NULL;
        }
        members = push_member(members, member, &num_members);
    } while (peek_next_nonspace(file, line_num) != '}');

    qsort(members, num_members, sizeof(JsonMember*), json_member_cmp); 

    return members;
}

static JsonObject* parse_object(const char* path, FILE* file, int* line_num, int* error_count)
{
    JsonObject* object;
    char c;

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

    c = peek_next_nonspace(file, line_num);
    if (c == '}') {
        getch(file, line_num);
        object = malloc(sizeof(JsonObject));
        ASSERT(object != NULL);
        object->members = malloc(sizeof(JsonMember));
        ASSERT(object->members != NULL);
        object->members[0] = NULL;
        return object;
    }

    JsonMember** members = parse_members(path, file, line_num, error_count);

    if (members == NULL) {
        print_error(path, line_num, "Error parsing objects");
        return NULL;
    }

    object = malloc(sizeof(JsonObject));
    ASSERT(object != NULL);
    object->members = (const JsonMember**)members;

    c = get_next_nonspace(file, line_num);

    if (c != '}') {
        print_error(path, line_num, "Expected '}'");
        json_object_destroy(object);
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
        json_object_destroy(object);
        object = NULL;
    }
    if (fclose(file) == EOF) {
        printf("Error reading json file: %s\n", path);
        json_object_destroy(object);
        return NULL;
    }
    if (object == NULL) {
        printf("\nParsing failed\n");
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

void json_object_destroy(JsonObject* object)
{
    UNUSED(object);
}

const char* negatives[] = {
    "missing_file",
    "missing_opening_brackets",
    "unrecognized_char",
    "missing_closing_brackets",
    "characters_after_root",
    "missing_endquote"
};

const char* positives[] = {
    "test1",
    "test2",
    "test3",
};

int main(void)
{
    JsonObject* obj;
    char str[256];
    int total = 0;
    int passed = 0;
    for (int i = 0; i < (int)(sizeof(negatives)/sizeof(char*)); i++) {
        sprintf(str, "tests/%s.json", negatives[i]);
        obj = json_read(str);
        if (obj == NULL)
            passed++;
        total++;
        json_object_destroy(obj);
        puts("----------------------------");
    }
    puts("");
    printf("Passed %d / %d\n", passed, total);
    puts("");
    puts("===========================");
    puts("");
    total = passed = 0;
    for (int i = 0; i < (int)(sizeof(positives)/sizeof(char*)); i++) {
        sprintf(str, "tests/%s.json", positives[i]);
        obj = json_read(str);
        if (obj != NULL)
            passed++;
        total++;
        json_object_destroy(obj);
        puts("----------------------------");
    }
    puts("");
    printf("Passed %d / %d\n", passed, total);
    return 0;
}
