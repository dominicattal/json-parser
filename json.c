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

static void print_error(const int* line_num, const char* message)
{
#ifdef JSON_VERBOSE
    printf("[%d]: %s\n", *line_num, message);
#else
    UNUSED(line_num);
    UNUSED(message);
#endif
}


// https://www.json.org/json-en.html

typedef struct JsonMember {
    char* key;
    JsonValue* value;
} JsonMember;

typedef struct JsonObject {
    JsonMember** members;
    int num_members;
} JsonObject;

typedef struct JsonArray {
    JsonValue** values;
    int num_values;
} JsonArray;

typedef struct JsonValue {
    JsonType type;
    union {
        JsonObject* val_object;
        JsonArray* val_array;
        char* val_string;
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

static void json_member_destroy(JsonMember* member);
static void json_value_destroy(JsonValue* value);
static void json_array_destroy(JsonArray* array);
static void json_values_destroy(JsonValue** values, int num_values);
static void json_members_destroy(JsonMember** members, int num_members);

static void json_object_print(const JsonObject* object, int depth);
static void json_array_print(const JsonArray* array, int depth);

static void json_value_destroy(JsonValue* value)
{
    if (value == NULL) return;
    switch (value->type) {
        case JTYPE_OBJECT:
            json_object_destroy(value->val_object);
            break;
        case JTYPE_ARRAY:
            json_array_destroy(value->val_array);
            break;
        case JTYPE_STRING:
            free(value->val_string);
            break;
        default:
            break;
    }
    free(value);
}

static void json_member_destroy(JsonMember* member)
{
    if (member == NULL) return;
    free(member->key);
    json_value_destroy(member->value);
    free(member);
}

static void json_members_destroy(JsonMember** members, int num_members)
{
    for (int i = 0; i < num_members; i++)
        json_member_destroy(members[i]);
    free(members);
}

static void json_values_destroy(JsonValue** values, int num_values)
{
    for (int i = 0; i < num_values; i++)
        json_value_destroy(values[i]);
    free(values);
}

static void json_array_destroy(JsonArray* array)
{
    if (array == NULL) return;
    json_values_destroy(array->values, array->num_values);
    free(array);
}

void json_object_destroy(JsonObject* object)
{
    if (object == NULL) return;
    json_members_destroy(object->members, object->num_members);
    free(object);
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
        list = malloc(size);
    else
        list = realloc(list, (*length+1) * size);
    ASSERT(list != NULL);
    list[(*length)++] = data;
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

static int json_ws(char c)
{
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

static char get_next_nonspace(FILE* file, int* line_num)
{
    char c;
    do { c = getch(file, line_num);
    } while (c != EOF && json_ws(c));
    return c;
}

static char peek_next_nonspace(FILE* file, int* line_num)
{
    char c = get_next_nonspace(file, line_num);
    ungetch(file, line_num, c);
    return c;
}

// gets substring from start_pos until end_pos, returns NULL if not valid
static char* get_string_in_range(FILE* file, int* line_num, long start_pos, long end_pos)
{
    if (fseek(file, start_pos, SEEK_SET) == -1) {
        print_error(line_num, "Something unexpected happened");
        return NULL;
    }
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

static int to_hex(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a';
    if (c >= 'A' && c <= 'F')
        return c - 'A';
    return -1;
}

static int is_escape_char(char c)
{
    return c == '"' || c == '\\' || c == '/'
        || c == 'b' || c == 'f' || c == 'n'
        || c == 'r' || c == 't' || c == 'u';
}

static char map_escape_sequence(char c)
{
    switch (c) {
        case '"': return '"';
        case '\\': return '\\';
        case '/': return '/';
        case 'b': return '\b';
        case 'f': return '\f';
        case 'n': return '\n';
        case 'r': return '\r';
        case 't': return '\t';
        default: break;
    }
    return -1;
}

// gets next string surrounded by quotes, returns NULL if not valid
static char* get_next_string(FILE* file, int* line_num)
{
    char c;
    c = get_next_nonspace(file, line_num);
    if (c != '"') {
        print_error(line_num, "Missing quotes");
        return NULL;
    }

    long start_pos = ftell(file);
    int count, hex, dig;
    count = 0;
    while (1) {
        count += 1;
        c = getch(file, line_num);
        if (c == '"') break;
        if (c == EOF) break;
        if (c != '\\') continue;
        c = getch(file, line_num);
        if (c == EOF) break;
        if (!is_escape_char(c)) {
            print_error(line_num, "Invalid escape sequence");
            return NULL;
        }
        if (c != 'u') continue;
        for (int i = 0; i < 3; i++) {
            c = getch(file, line_num);
            if (c == EOF) {
                print_error(line_num, "Expected hex literal");
                return NULL;
            }
        }
    }

    if (c == EOF) {
        print_error(line_num, "Expected ending quotes for string");
        return NULL;
    }

    char* string = malloc(count * sizeof(char));
    ASSERT(string != NULL);
    if (fseek(file, start_pos, SEEK_SET) == -1) {
        print_error(line_num, "Something unexpected happened");
        return NULL;
    }

    count = 0;
    while (1) {
        c = getch(file, line_num);
        ASSERT(c != EOF);
        if (c == '"') break;
        if (c != '\\') {
            string[count++] = c;
            continue;
        }
        c = getch(file, line_num);
        ASSERT(c != EOF);
        if (c != 'u') {
            string[count++] = map_escape_sequence(c);
            continue;
        }
        dig = to_hex(c);
        if (dig == -1) {
            print_error(line_num, "Invalid hex literal");
            free(string);
            return NULL;
        }
        hex = dig << 12;
        for (int i = 2; i >= 0; i--) {
            c = getch(file, line_num);
            printf("%d(%c)\n", c, c);
            ASSERT(c != EOF);
            dig = to_hex(c);
            if (dig == -1) {
                print_error(line_num, "Invalid hex literal");
                free(string);
                return NULL;
            }
            hex |= dig << (4*i);
        }
        string[count++] = hex;
    }

    string[count] = '\0';
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

static JsonValue** parse_values(FILE* file, int* line_num, int* num_values)
{
    char c;
    JsonValue** values = NULL;
    JsonValue* value;

    value = parse_value(file, line_num);
    if (value == NULL) {
        print_error(line_num, "Error parsing value");
        return NULL;
    }

    values = push_value(values, value, num_values);
    c = peek_next_nonspace(file, line_num);
    if (c == ']')
        return values;

    do {
        c = get_next_nonspace(file, line_num);
        if (c != ',') {
            print_error(line_num, "Missing comma between values");
            json_values_destroy(values, *num_values);
            return NULL;
        }
        value = parse_value(file, line_num);
        if (value == NULL) {
            print_error(line_num, "Error parsing value");
            json_values_destroy(values, *num_values);
            return NULL;
        }
        values = push_value(values, value, num_values);
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
        array->num_values = 0;
        return array;
    }
 
    int num_values = 0;
    JsonValue** values = parse_values(file, line_num, &num_values);

    if (values == NULL) {
        print_error(line_num, "Error parsing array");
        return NULL;
    }

    array = malloc(sizeof(JsonArray));
    ASSERT(array != NULL);
    array->values = values;
    array->num_values = num_values;

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

static int next_state_number(int state, char c)
{
    switch (state) {
        case 0:
            if (c == '-') return 1;
            if (c == '0') return 2;
            if (isdigit(c)) return 3;
            break;
        case 1:
            if (c == '0') return 2;
            if (isdigit(c)) return 3;
            break;
        case 2:
            if (c == '.') return 4;
            if (c == 'e') return 6;
            if (c == 'E') return 6;
            break;
        case 3:
            if (c == '.') return 4;
            if (c == 'e') return 6;
            if (c == 'E') return 6;
            if (isdigit(c)) return 3;
            break;
        case 4:
            if (isdigit(c)) return 5;
            break;
        case 5:
            if (c == 'e') return 6;
            if (c == 'E') return 6;
            if (isdigit(c)) return 5;
            break;
        case 6:
            if (c == '+') return 7;
            if (c == '-') return 7;
            if (isdigit(c)) return 8;
            break;
        case 7:
            if (isdigit(c)) return 8;
            break;
        case 8:
            if (isdigit(c)) return 8;
            break;
        default:
            break;
    }
    return -1;
}

static long dfa_number(FILE* file, int* line_num, JsonType* type)
{
    int state = 0, prev_state;
    char c;
    while ((c = getch(file, line_num)) != EOF) {
        prev_state = state;
        state = next_state_number(state, c);
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
    char* string = get_next_string(file, line_num);
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
    value->type = JTYPE_FALSE;
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
    value->type = JTYPE_NULL;
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
    char* key = get_next_string(file, line_num);
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

static JsonMember** parse_members(FILE* file, int* line_num, int* num_members)
{
    char c;
    JsonMember** members = NULL;
    JsonMember* member;

    member = parse_member(file, line_num);
    if (member == NULL) {
        print_error(line_num, "Error parsing member");
        return NULL;
    }

    members = push_member(members, member, num_members);
    c = peek_next_nonspace(file, line_num);
    if (c == '}')
        return members;

    do {
        c = get_next_nonspace(file, line_num);
        if (c != ',') {
            print_error(line_num, "Missing comma between members");
            json_members_destroy(members, *num_members);
            return NULL;
        }
        member = parse_member(file, line_num);
        if (member == NULL) {
            print_error(line_num, "Error parsing member");
            json_members_destroy(members, *num_members);
            return NULL;
        }
        members = push_member(members, member, num_members);
    } while (peek_next_nonspace(file, line_num) != '}');

    qsort(members, *num_members, sizeof(JsonMember*), json_member_cmp); 

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
        object->num_members = 0;
        return object;
    }

    int num_members = 0;
    JsonMember** members = parse_members(file, line_num, &num_members);

    if (members == NULL) {
        print_error(line_num, "Error parsing object");
        return NULL;
    }

    object = malloc(sizeof(JsonObject));
    ASSERT(object != NULL);
    object->members = members;
    object->num_members = num_members;

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
    FILE* file = fopen(path, "r");
    if (file == NULL)
        return NULL;

    int line_num = 1;
    JsonObject* object = parse_object(file, &line_num);
    if (object != NULL && get_next_nonspace(file, &line_num) != EOF) {
        print_error(&line_num, "Excess characters after root");
        json_object_destroy(object);
        object = NULL;
    }

    if (fclose(file) == EOF) {
        json_object_destroy(object);
        return NULL;
    }

    return object;
}

JsonValue* json_get_value(const JsonObject* object, const char* key)
{
    JsonMember* member;
    int m, l, r, a;
    l = 0;
    r = object->num_members - 1;
    while (l <= r) {
        m = l + (r - l) / 2;
        member = object->members[m];
        a = strcmp(key, member->key);
        if (a > 0) 
            l = m + 1;
        else if (a < 0) 
            r = m - 1;
        else 
            return member->value;
    }
    return NULL;
}

JsonType json_get_type(const JsonValue* value)
{
    return value->type;
}

JsonObject* json_get_object(const JsonValue* value)
{
    return value->val_object;
}

JsonArray* json_get_array(const JsonValue* value)
{
    return value->val_array;
}

char* json_get_string(const JsonValue* value)
{
    return value->val_string;
}

int json_get_int(const JsonValue* value)
{
    return value->val_int;
}

float json_get_float(const JsonValue* value)
{
    return value->val_float;
}

int json_array_length(const JsonArray* array)
{
    return array->num_values;
}

JsonValue* json_array_get(const JsonArray* array, int idx)
{
    return array->values[idx];
}

static void tab(int depth)
{
    for (int j = 0; j < depth; j++)
        printf("  ");
}

static void print_string(char* string)
{
    printf("\"");
    for (int i = 0; string[i] != '\0'; i++) {
        if (string[i] == '"' || string[i] == '\\' || string[i] == '/')
            printf("\\");
        printf("%c", string[i]);
    }
    printf("\"");
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
            print_string(value->val_string);
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
    if (array->num_values == 0) {
        printf("]");
        return;
    }
    printf("\n");

    const JsonValue* value;
    value = array->values[0];
    tab(depth+1);
    json_value_print(value, depth+1);

    for (int i = 1; i < array->num_values; i++) {
        value = array->values[i];
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
    if (object->num_members == 0) {
        printf("}");
        return;
    }
    printf("\n");

    const JsonMember* member;
    member = object->members[0];
    tab(depth+1);
    printf("\"%s\": ", member->key);
    json_value_print(member->value, depth+1);

    for (int i = 1; i < object->num_members; i++) {
        member = object->members[i];
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

void json_print_array(const JsonArray* array)
{
    json_array_print(array, 0);
    puts("");
}

void json_print_value(const JsonValue* value)
{
    json_value_print(value, 0);
    puts("");
}
