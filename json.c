#include "json.h"
#include <dirent.h>
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
        const JsonObject* val_object;
        const JsonArray* val_array;
        const char* val_string;
        const int val_int;
        const float val_float;
    };
} JsonValue;

static JsonValue* parse_value(FILE* file, int* line_num);
static JsonValue* parse_value_object(FILE* file, int* line_num);
static JsonValue* parse_value_array(FILE* file, int* line_num);
static JsonValue* parse_value_number(FILE* file, int* line_num);
static JsonValue* parse_value_string(FILE* file, int* line_num);
static JsonValue* parse_value_true(FILE* file, int* line_num);
static JsonValue* parse_value_false(FILE* file, int* line_num);
static JsonValue* parse_value_null(FILE* file, int* line_num);
static JsonObject* parse_object(FILE* file, int* line_num);
static JsonMember* parse_member(FILE* file, int* line_num);
static JsonMember** parse_members(FILE* file, int* line_num);

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

static void print_error(const int* line_num, const char* message)
{
    printf("[%d]: %s\n", *line_num, message);
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
static const char* get_next_string(FILE* file, int* line_num)
{
    char c;
    long start_pos, end_pos;

    c = get_next_nonspace(file, line_num);
    if (c != '"') {
        print_error(line_num, "Missing quotes");
        return NULL;
    }

    start_pos = ftell(file);
    ASSERT(start_pos != -1);
    end_pos = get_next_char_pos(file, line_num, '"');
    ASSERT(end_pos != -1);
    if (end_pos == -2) {
        print_error(line_num, "Expected closing quotes");
        return NULL;
    }

    fseek(file, start_pos, SEEK_SET);
    int n = end_pos - start_pos;
    char* string = malloc(n * sizeof(char));
    fgets(string, n+1, file);
    string[n-1] = '\0';
    return string;
}

static JsonValue* parse_value_object(FILE* file, int* line_num)
{
    UNUSED(file);
    UNUSED(line_num);
    return NULL;
}

static JsonValue* parse_value_array(FILE* file, int* line_num)
{
    UNUSED(file);
    UNUSED(line_num);
    return NULL;
}

static JsonValue* parse_value_number(FILE* file, int* line_num)
{
    UNUSED(file);
    UNUSED(line_num);
    return NULL;
}

static JsonValue* parse_value_string(FILE* file, int* line_num)
{

    const char* string = get_next_string(file, line_num);
    if (string == NULL) {
        print_error(line_num, "Error parsing value string");
        return NULL;
    }

    JsonValue* value = malloc(sizeof(JsonValue));
    value->type = JTYPE_STRING;
    value->val_string = string;

    return value;
}

static JsonValue* parse_value_true(FILE* file, int* line_num)
{
    UNUSED(line_num);
    char str[5];
    fgets(str, 5, file);
    str[4] = '\0';
    if (strcmp(str, "true"))
        return NULL;
    JsonValue* value = malloc(sizeof(JsonValue));
    value->type = JTYPE_TRUE;
    return value;
}

static JsonValue* parse_value_false(FILE* file, int* line_num)
{
    UNUSED(line_num);
    char str[6];
    fgets(str, 6, file);
    str[5] = '\0';
    if (strcmp(str, "false"))
        return NULL;
    JsonValue* value = malloc(sizeof(JsonValue));
    value->type = JTYPE_TRUE;
    return value;
}

static JsonValue* parse_value_null(FILE* file, int* line_num)
{
    UNUSED(line_num);
    char str[5];
    fgets(str, 5, file);
    str[4] = '\0';
    if (strcmp(str, "null"))
        return NULL;
    JsonValue* value = malloc(sizeof(JsonValue));
    value->type = JTYPE_TRUE;
    return value;
}

static JsonValue* parse_value(FILE* file, int* line_num)
{
    char c = peek_next_nonspace(file, line_num);
    if (c == '[')
        return parse_value_array(file, line_num);
    if (c == '{')
        return parse_value_object(file, line_num);
    if (c == '-' || isdigit(c))
        return parse_value_number(file, line_num);
    if (c == '"')
        return parse_value_string(file, line_num);
    if (c == 't')
        return parse_value_true(file, line_num);
    if (c == 'f')
        return parse_value_false(file, line_num);
    if (c == 'n')
        return parse_value_null(file, line_num);
    print_error(line_num, "Invalid value");
    return NULL;
}

static JsonMember* parse_member(FILE* file, int* line_num)
{
    char c;
    
    const char* key = get_next_string(file, line_num);
    if (key == NULL) {
        print_error(line_num, "Error reading key");
        return NULL;
    }

    c = get_next_nonspace(file, line_num);
    if (c != ':') {
        print_error(line_num, "Missing colon");
        return NULL;
    }

    JsonValue* value;
    value = parse_value(file, line_num);
    if (value == NULL) {
        print_error(line_num, "Error reading value");
        return NULL;
    }

    JsonMember* member = malloc(sizeof(JsonMember));
    ASSERT(member != NULL);
    member->key = key;
    member->value = value;

    return member;
}

static JsonMember** parse_members(FILE* file, int* line_num)
{
    char c;
    int num_members = 0;
    JsonMember** members = NULL;
    JsonMember* member;

    member = parse_member(file, line_num);
    if (member == NULL) {
        print_error(line_num, "Error parsing member");
        return NULL;
    }

    members = push_member(members, member, &num_members);
    c = peek_next_nonspace(file, line_num);
    if (c == '}')
        return members;

    do {
        c = get_next_nonspace(file, line_num);
        if (c != ',') {
            print_error(line_num, "Missing comma between members");
            json_members_destroy(members);
            return NULL;
        }
        member = parse_member(file, line_num);
        if (member == NULL) {
            print_error(line_num, "Error parsing member");
            json_members_destroy(members);
            return NULL;
        }
        members = push_member(members, member, &num_members);
    } while (peek_next_nonspace(file, line_num) != '}');

    qsort(members, num_members, sizeof(JsonMember*), json_member_cmp); 

    return members;
}

static JsonObject* parse_object(FILE* file, int* line_num)
{
    JsonObject* object;
    char c;

    c = get_next_nonspace(file, line_num);
    if (c == EOF) {
        print_error(line_num, "Expected '{'");
        return NULL;
    }
    if (c != '{') {
        if (c == '}')
            print_error(line_num, "Missing '{'");
        else
            print_error(line_num, "Unexpected character before '{'");
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

    JsonMember** members = parse_members(file, line_num);

    if (members == NULL) {
        print_error(line_num, "Error parsing objects");
        return NULL;
    }

    object = malloc(sizeof(JsonObject));
    ASSERT(object != NULL);
    object->members = (const JsonMember**)members;

    c = get_next_nonspace(file, line_num);

    if (c != '}') {
        print_error(line_num, "Expected '}'");
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
    JsonObject* object = parse_object(file, &line_num);
    if (object != NULL && get_next_nonspace(file, &line_num) != EOF) {
        print_error(&line_num, "Excess characters");
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

void json_object_print(const JsonObject* object, int depth);

void json_object_print_member(const JsonMember* member, int depth)
{
    UNUSED(depth);
    ASSERT(member != NULL);
    ASSERT(member->key != NULL);
    ASSERT(member->value != NULL);
    int i;
    const char* key = member->key;
    JsonValue* value = member->value;
    for (i = 0; i < depth; i++)
        printf("  ");
    printf("%s: ", key);
    switch (value->type) {
        case JTYPE_TRUE:
            puts("true");
            break;
        case JTYPE_FALSE:
            puts("false");
            break;
        case JTYPE_NULL:
            puts("null");
            break;
        case JTYPE_STRING:
            puts(value->val_string);
            break;
        case JTYPE_OBJECT:
            json_object_print(value->val_object, depth+1);
            break;
        default:
            break;
    }
}

void json_object_print(const JsonObject* object, int depth)
{
    printf("{\n");
    int i;
    for (i = 0; i < depth; i++)
        printf("  ");
    ASSERT(object->members != NULL);
    i = 0;
    const JsonMember* member;
    while ((member = object->members[i++]) != NULL)
        json_object_print_member(member, depth+1);
    for (i = 0; i < depth; i++)
        printf("  ");
    printf("}\n");
}

void json_object_destroy(JsonObject* object)
{
    UNUSED(object);
}

void test(const char* path)
{
    char str[512];
    DIR* negatives = opendir(path);
    assert(negatives != NULL);
    struct dirent* entry;
    while ((entry = readdir(negatives)) != NULL) {
        if (!strcmp(entry->d_name, "."))
            continue;
        if (!strcmp(entry->d_name, ".."))
            continue;
        sprintf(str, "%s/%s", path, entry->d_name);
        JsonObject* object = json_read(str);
        if (object != NULL) {
            puts("");
            json_object_print(object, 0);
        }
        json_object_destroy(object);
        puts("-------------------------");
    }
}

int main(void)
{
    test("negatives");
    test("positives");
    return 0;
}
