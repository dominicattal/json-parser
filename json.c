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

typedef struct JsonArray {
    const JsonValue** values;
} JsonArray;

typedef struct JsonValue {
    JsonType type;
    union {
        const JsonObject* val_object;
        const JsonArray* val_array;
        const char* val_string;
        double val_float;
        long long val_int;
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

static void json_values_destroy(JsonValue** values)
{
    UNUSED(values);
}

static void json_array_destroy(JsonArray* array)
{
    UNUSED(array);
}

void json_object_destroy(JsonObject* object)
{
    UNUSED(object);
}

static int json_member_cmp(const void* x, const void* y)
{
    const JsonMember* const* m1 = x;
    const JsonMember* const* m2 = y;
    return strcmp((*m1)->key, (*m2)->key);
}

static void* push_data(void** list, void* data, int* length, size_t size)
{
    if (list == NULL)
        list = malloc(2 * size);
    else
        list = realloc(list, (*length+2) * size);
    ASSERT(list != NULL);
    list[(*length)++] = data;
    list[(*length)] = NULL;
    return list;
}

static JsonMember** push_member(JsonMember** members, JsonMember* member, int* length)
{   
    return push_data((void**)members, (void*)member, length, sizeof(JsonMember*));
}

static JsonValue** push_value(JsonValue** values, JsonValue* value, int* length)
{
    return push_data((void**)values, (void*)value, length, sizeof(JsonValue*));
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

// gets substring from start_pos until end_pos, returns NULL if not valid
static const char* get_string_in_range(FILE* file, int* line_num, long start_pos, long end_pos)
{
    fseek(file, start_pos, SEEK_SET);
    int n = end_pos - start_pos;
    char* string = malloc((n+1) * sizeof(char));
    ASSERT(string != NULL);
    if (fgets(string, n+1, file) == NULL) {
        free(string);
        print_error(line_num, "Something went wrong reading from file");
        return NULL;
    }
    string[n] = '\0';
    return string;
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

    const char* string = get_string_in_range(file, line_num, start_pos, end_pos-1);
    getch(file, line_num);
    if (string == NULL)
        return NULL;
    return string;
}

static JsonValue* parse_value_object(FILE* file, int* line_num)
{
    JsonObject* object = parse_object(file, line_num);
    if (object == NULL) {
        print_error(line_num, "Error reading value object");
        return NULL;
    }
    JsonValue* value = malloc(sizeof(JsonValue));
    ASSERT(value != NULL);
    value->type = JTYPE_OBJECT;
    value->val_object = object;
    return value;
}

static JsonValue** parse_values(FILE* file, int* line_num)
{
    char c;
    int num_values = 0;
    JsonValue** values = NULL;
    JsonValue* value;

    value = parse_value(file, line_num);
    if (value == NULL) {
        print_error(line_num, "Error parsing value");
        return NULL;
    }

    values = push_value(values, value, &num_values);
    c = peek_next_nonspace(file, line_num);
    if (c == ']')
        return values;

    do {
        c = get_next_nonspace(file, line_num);
        if (c != ',') {
            print_error(line_num, "Missing comma between values");
            json_values_destroy(values);
            return NULL;
        }
        value = parse_value(file, line_num);
        if (value == NULL) {
            print_error(line_num, "Error parsing value");
            json_values_destroy(values);
            return NULL;
        }
        values = push_value(values, value, &num_values);
    } while (peek_next_nonspace(file, line_num) != ']');

    return values;
}

static JsonArray* parse_array(FILE* file, int* line_num)
{
    JsonArray* array;
    char c;
    c = get_next_nonspace(file, line_num);
    if (c == EOF) {
        print_error(line_num, "Expected '['");
        return NULL;
    }
    if (c != '[') {
        if (c == ']')
            print_error(line_num, "Missing ']'");
        else
            print_error(line_num, "Unexpected character before '['");
        return NULL;
    }
    
    c = peek_next_nonspace(file, line_num);
    if (c == ']') {
        getch(file, line_num);
        array = malloc(sizeof(JsonArray));
        ASSERT(array != NULL);
        array->values = malloc(sizeof(JsonValue));
        ASSERT(array->values != NULL);
        array->values[0] = NULL;
        return array;
    }
 
    JsonValue** values = parse_values(file, line_num);

    if (values == NULL) {
        print_error(line_num, "Error parsing array");
        return NULL;
    }

    array = malloc(sizeof(JsonArray));
    ASSERT(array != NULL);
    array->values = (const JsonValue**)values;

    c = get_next_nonspace(file, line_num);

    if (c != ']') {
        print_error(line_num, "Expected ']'");
        json_array_destroy(array);
        return NULL;
    }

    return array;
}

static JsonValue* parse_value_array(FILE* file, int* line_num)
{
    JsonArray* array = parse_array(file, line_num);
    if (array == NULL) {
        print_error(line_num, "Error reading value array");
        return NULL;
    }
    JsonValue* value = malloc(sizeof(JsonValue));
    ASSERT(value != NULL);
    value->type = JTYPE_ARRAY;
    value->val_array = array;
    return value;
}

static int accepting_state_int(int state)
{
    return state == 2 || state == 3;
}

static int accepting_state_float(int state)
{
    return state == 5 || state == 8;
}

static int next_state(int state, char c)
{
    switch (state) {
        case 0:
            if (c == '-') return 1;
            if (c == '0') return 2;
            if (isdigit(c)) return 3;
            return -1;
        case 1:
            if (c == '0') return 2;
            if (isdigit(c)) return 3;
            return -1;
        case 2:
            if (c == '.') return 4;
            if (c == 'e') return 6;
            if (c == 'E') return 6;
            return -1;
        case 3:
            if (c == '.') return 4;
            if (c == 'e') return 6;
            if (c == 'E') return 6;
            if (isdigit(c)) return 3;
            return -1;
        case 4:
            if (isdigit(c)) return 5;
            return -1;
        case 5:
            if (c == 'e') return 6;
            if (c == 'E') return 6;
            if (isdigit(c)) return 5;
            return -1;
        case 6:
            if (c == '+') return 7;
            if (c == '-') return 7;
            if (isdigit(c)) return 8;
            return -1;
        case 7:
            if (isdigit(c)) return 8;
            return -1;
        case 8:
            if (isdigit(c)) return 8;
            return -1;
        default:
            return -1;
    }
    return -1;
}

static long dfa_number(FILE* file, int* line_num, JsonType* type)
{
    UNUSED(file);
    UNUSED(line_num);
    int state = 0, prev_state;
    char c;
    while ((c = getch(file, line_num)) != EOF) {
        prev_state = state;
        state = next_state(state, c);
        if (state == -1) {
            ungetch(file, line_num, c);
            if (accepting_state_int(prev_state)) {
                *type = JTYPE_INT;
                return ftell(file);
            }
            if (accepting_state_float(prev_state)) {
                *type = JTYPE_FLOAT;
                return ftell(file);
            }
            return -2;
        }
    }
    return -2;
}

static JsonValue* parse_value_number(FILE* file, int* line_num)
{
    JsonType type;
    long start_pos, end_pos;
    start_pos = ftell(file);
    ASSERT(start_pos != -1);
    end_pos = dfa_number(file, line_num, &type);
    ASSERT(end_pos != -1);
    if (end_pos == -2) {
        print_error(line_num, "Error parsing value num");
        return NULL;
    }

    const char* string = get_string_in_range(file, line_num, start_pos, end_pos);
    if (string == NULL)
        return NULL;

    double num = atof(string);

    JsonValue* value = malloc(sizeof(JsonValue));
    ASSERT(value != NULL);
    value->type = type;
    if (type == JTYPE_INT)
        value->val_int = num;
    else
        value->val_float = num;
    
    return value;
}

static JsonValue* parse_value_string(FILE* file, int* line_num)
{

    const char* string = get_next_string(file, line_num);
    if (string == NULL) {
        print_error(line_num, "Error parsing value string");
        return NULL;
    }

    JsonValue* value = malloc(sizeof(JsonValue));
    ASSERT(value != NULL);
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
    ASSERT(value != NULL);
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
    ASSERT(value != NULL);
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
    ASSERT(value != NULL);
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
        print_error(line_num, "Error parsing object");
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

static void json_object_print(const JsonObject* object, int depth);
static void json_array_print(const JsonArray* array, int depth);

static void tab(int depth)
{
    for (int j = 0; j < depth; j++)
        printf("  ");
}

static void json_value_print(const JsonValue* value, int depth)
{
    ASSERT(value != NULL);
    switch (value->type) {
        case JTYPE_TRUE:
            printf("true");
            break;
        case JTYPE_FALSE:
            printf("false");
            break;
        case JTYPE_NULL:
            printf("null");
            break;
        case JTYPE_STRING:
            printf("\"%s\"", value->val_string);
            break;
        case JTYPE_INT:
            printf("%lld", value->val_int);
            break;
        case JTYPE_FLOAT:
            printf("%f", value->val_float);
            break;
        case JTYPE_OBJECT:
            json_object_print(value->val_object, depth);
            break;
        case JTYPE_ARRAY:
            json_array_print(value->val_array, depth);
            break;
        default:
            break;
    }
}

static void json_array_print(const JsonArray* array, int depth)
{
    ASSERT(array->values != NULL);
    printf("[");
    if (array->values[0] == NULL) {
        printf("]");
        return;
    }
    printf("\n");

    int i = 0;
    const JsonValue* value;
    value = array->values[i++];
    tab(depth+1);
    json_value_print(value, depth+1);

    while ((value = array->values[i++]) != NULL) {
        printf(",\n");
        tab(depth+1);
        json_value_print(value, depth+1);
    }
    printf("\n");
    tab(depth);
    printf("]");
}

static void json_object_print(const JsonObject* object, int depth)
{
    ASSERT(object->members != NULL);
    printf("{");
    if (object->members[0] == NULL) {
        printf("}");
        return;
    }
    printf("\n");

    int i = 0;
    const JsonMember* member;
    member = object->members[i++];
    tab(depth+1);
    printf("\"%s\": ", member->key);
    json_value_print(member->value, depth+1);

    while ((member = object->members[i++]) != NULL) {
        printf(",\n");
        tab(depth+1);
        printf("\"%s\": ", member->key);
        json_value_print(member->value, depth+1);
    }
    printf("\n");
    tab(depth);
    printf("}");
}

void json_print_object(const JsonObject* object)
{
    json_object_print(object, 0);
    puts("");
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
            json_print_object(object);
        }
        json_object_destroy(object);
        puts("-------------------------");
    }
}

int main(void)
{
    test("negatives");
    //test("positives");
    return 0;
}
