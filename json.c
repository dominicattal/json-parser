#include "json.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// https://www.json.org/json-en.html

// can change these
#define print_error(line_num, message)
#define json_malloc(size)               malloc(size)
#define json_realloc(ptr, size)         realloc(ptr, size)
#define json_free(ptr)                  free(ptr)

struct JsonMember {
    char* key;
    JsonValue* value;
};

struct JsonObject {
    JsonMember** members;
    int num_members;
};

struct JsonArray {
    JsonValue** values;
    int num_values;
};

struct JsonValue {
    JsonType type;
    union {
        JsonObject* _object;
        JsonArray* _array;
        char* _string;
        JsonFloat _float;
        JsonInt _int;
    } val;
};

struct JsonIterator {
    const JsonObject* object;
    int idx;
};

// ---- Some helper functions defined below main api ------
static JsonValue*   parse_value(FILE* file, int* line_num);
static JsonValue*   parse_value_object(FILE* file, int* line_num);
static JsonValue*   parse_value_array(FILE* file, int* line_num);
static JsonValue*   parse_value_number(FILE* file, int* line_num);
static JsonValue*   parse_value_string(FILE* file, int* line_num);
static JsonValue*   parse_value_true(FILE* file, int* line_num);
static JsonValue*   parse_value_false(FILE* file, int* line_num);
static JsonValue*   parse_value_null(FILE* file, int* line_num);
static JsonObject*  parse_object(FILE* file, int* line_num);
static JsonMember*  parse_member(FILE* file, int* line_num);
static char         get_next_nonspace(FILE* file, int* line_num);

static void         json_members_destroy(JsonMember** members, int num_members);
static void         json_values_destroy(JsonValue** values, int num_values);

static void*        push_data(void** list, void* data, int* length, size_t size);
static JsonMember** push_member(JsonMember** members, JsonMember* member, int* length);
static JsonValue**  push_value(JsonValue** values, JsonValue* value, int* length);

static void         print_object(FILE* fptr, const JsonObject* object, int depth);
static void         print_value(FILE* fptr, const JsonValue* value, int depth);
static void         print_array(FILE* fptr, const JsonArray* array, int depth);

// ------------------- Objects ------------------------

JsonObject* json_read(const char* path)
{
    FILE* file;
    int line_num;
    JsonObject* object;

    file = fopen(path, "r");
    if (file == NULL)
        return NULL;

    line_num = 1;
    object = parse_object(file, &line_num);
    if (object != NULL && get_next_nonspace(file, &line_num) != EOF) {
        print_error(&line_num, "Excess characters after root");
        json_object_destroy(object);
        object = NULL;
    }

    if (fclose(file) == EOF) {
        print_error(&line_num, "Unexpected error when closing file");
        json_object_destroy(object);
        object = NULL;
    }

    return object;
}

JsonObject* json_object_create(void)
{
    JsonObject* object;
    object = json_malloc(sizeof(JsonObject));
    if (object == NULL)
        return NULL;
    object->members = NULL;
    object->num_members = 0;
    return object;
}

JsonObject* json_merge_objects(JsonObject* object1, JsonObject* object2)
{
    JsonObject* object3;
    JsonMember* member1;
    JsonMember* member2;
    JsonIterator* it1;
    JsonIterator* it2;
    const char* key1;
    const char* key2;
    int num_members1, num_members2, idx;
    int cmp;

    it1 = json_iterator_create(object1);
    if (it1 == NULL)
        return NULL;
    
    it2 = json_iterator_create(object2);
    if (it2 == NULL) {
        json_iterator_destroy(it1);
        return NULL;
    }

    object3 = json_malloc(sizeof(JsonObject));
    if (object3 == NULL) {
        json_iterator_destroy(it1);
        json_iterator_destroy(it2);
        return NULL;
    }

    num_members1 = json_object_length(object1);
    num_members2 = json_object_length(object2);
    object3->num_members = num_members1 + num_members2;
    object3->members = json_malloc(object3->num_members * sizeof(JsonMember*));
    if (object3->members == NULL) {
        json_free(object3);
        json_iterator_destroy(it1);
        json_iterator_destroy(it2);
        return NULL;
    }

    idx = 0;
    member1 = json_iterator_get(it1);
    member2 = json_iterator_get(it2);
    while (member1 != NULL && member2 != NULL) {
        key1 = json_member_get_key(member1);
        key2 = json_member_get_key(member2);
        cmp = strcmp(key1, key2);
        if (cmp < 0) {
            object3->members[idx++] = member1;
            json_iterator_increment(it1);
            member1 = json_iterator_get(it1);
        } else {
            object3->members[idx++] = member2;
            json_iterator_increment(it2);
            member2 = json_iterator_get(it2);
        }
    }
    while (member1 != NULL) {
        object3->members[idx++] = member1;
        json_iterator_increment(it1);
        member1 = json_iterator_get(it1);
    }
    while (member2 != NULL) {
        object3->members[idx++] = member2;
        json_iterator_increment(it2);
        member2 = json_iterator_get(it2);
    }

    json_free(object1->members);
    json_free(object2->members);
    json_free(object1);
    json_free(object2);
    json_iterator_destroy(it1);
    json_iterator_destroy(it2);
    
    return object3;
}

int json_object_length(const JsonObject* object)
{
    return object->num_members;
}

JsonMember* json_object_get_member(const JsonObject* object, const char* key)
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
            return member;
    }
    return NULL;
}

JsonValue* json_object_get_value(const JsonObject* object, const char* key)
{
    JsonMember* member = json_object_get_member(object, key);
    if (member == NULL) 
        return NULL;
    return member->value;
}

int json_object_attach(JsonObject* object, JsonMember* member)
{
    JsonMember** new_members;
    JsonMember* test_member;
    int m, l, r, a;
    l = m = 0;
    r = object->num_members - 1;
    while (l <= r) {
        m = l + (r - l) / 2;
        test_member = object->members[m];
        a = strcmp(member->key, test_member->key);
        if (a > 0) 
            l = m + 1;
        else if (a < 0) 
            r = m - 1;
        else
            return 1;
    }
    if (object->num_members == 0)
        new_members = json_malloc(sizeof(JsonMember*));
    else
        new_members = json_realloc(object->members, (object->num_members+1)  * sizeof(JsonMember*));
    if (new_members == NULL)
        return 2;
    for (r = object->num_members; r > l; r--)
        new_members[r] = new_members[r-1];
    new_members[l] = member;
    object->members = new_members;
    object->num_members++;
    return 0;
}

JsonMember* json_object_detach(JsonObject* object, const char* key)
{
    JsonMember** new_members;
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
        else  {
            while (m < object->num_members-1) {
                object->members[m] = object->members[m+1];
                m++;
            }
            if (object->num_members == 1) {
                json_free(object->members);
                new_members = NULL;
            } else {
                new_members = json_realloc(object->members, (object->num_members-1)  * sizeof(JsonMember*));
                if (new_members == NULL)
                    return NULL;
            }
            object->members = new_members;
            object->num_members--;
            return member;
        }
    }
    return NULL;
}

void json_object_detach_and_destroy(JsonObject* object, const char* key)
{
    json_member_destroy(json_object_detach(object, key));
}

void json_object_destroy(JsonObject* object)
{
    if (object == NULL) return;
    json_members_destroy(object->members, object->num_members);
    json_free(object);
}

void json_object_print(const JsonObject* object)
{
    print_object(stdout, object, 0);
    puts("");
}

int json_object_write(const JsonObject* object, const char* filename)
{
    FILE* fptr;
    fptr = fopen(filename, "w");
    if (fptr == NULL)
        return 1;
    print_object(fptr, object, 0);
    fprintf(fptr, "\n");
    fclose(fptr);
    return 0;
}

// ------------------ Members -------------------------

JsonMember* json_member_create(const char* key, JsonValue* value)
{
    JsonMember* member;
    int n;
    n = strlen(key);
    member = json_malloc(sizeof(JsonMember));
    member->key = json_malloc((n+1) * sizeof(char));
    memcpy(member->key, key, (n+1) * sizeof(char));
    member->value = value;
    return member;
}

char* json_member_get_key(const JsonMember* member)
{
    return member->key;
}

JsonValue* json_member_get_value(const JsonMember* member)
{
    return member->value;
}

void json_member_update(JsonMember* member, JsonValue* value)
{
    json_value_destroy(member->value);
    member->value = value;
}

void json_member_destroy(JsonMember* member)
{
    if (member == NULL) return;
    json_free(member->key);
    json_value_destroy(member->value);
    json_free(member);
}

void json_member_print(JsonMember* member)
{
    printf("\"%s\": ", member->key);
    json_value_print(member->value);
}

// ------------------- Value ------------------------

JsonValue* json_value_create_null(void)
{
    JsonValue* value = json_malloc(sizeof(JsonValue));
    if (value == NULL) return NULL;
    value->type = JTYPE_NULL;
    return value;
}

JsonValue* json_value_create_true(void)
{
    JsonValue* value = json_malloc(sizeof(JsonValue));
    if (value == NULL) return NULL;
    value->type = JTYPE_TRUE;
    return value;
}

JsonValue* json_value_create_false(void)
{
    JsonValue* value = json_malloc(sizeof(JsonValue));
    if (value == NULL) return NULL;
    value->type = JTYPE_FALSE;
    return value;
}

JsonValue* json_value_create_int(JsonInt val)
{
    JsonValue* value = json_malloc(sizeof(JsonValue));
    if (value == NULL) return NULL;
    value->type = JTYPE_INT;
    value->val._int = val;
    return value;
}

JsonValue* json_value_create_float(JsonFloat val)
{
    JsonValue* value = json_malloc(sizeof(JsonValue));
    if (value == NULL) return NULL;
    value->type = JTYPE_FLOAT;
    value->val._float = val;
    return value;
}

JsonValue* json_value_create_string(char* val)
{
    JsonValue* value = json_malloc(sizeof(JsonValue));
    if (value == NULL) return NULL;
    value->type = JTYPE_STRING;
    value->val._string = val;
    return value;
}

JsonValue* json_value_create_object(JsonObject* val)
{
    JsonValue* value = json_malloc(sizeof(JsonValue));
    if (value == NULL) return NULL;
    value->type = JTYPE_OBJECT;
    value->val._object = val;
    return value;
}

JsonValue* json_value_create_array(JsonArray* val)
{
    JsonValue* value = json_malloc(sizeof(JsonValue));
    if (value == NULL) return NULL;
    value->type = JTYPE_ARRAY;
    value->val._array = val;
    return value;
}

JsonType json_value_get_type(const JsonValue* value)
{
    return value->type;
}

JsonObject* json_value_get_object(const JsonValue* value)
{
    return value->val._object;
}

JsonArray* json_value_get_array(const JsonValue* value)
{
    return value->val._array;
}

char* json_value_get_string(const JsonValue* value)
{
    return value->val._string;
}

JsonInt json_value_get_int(const JsonValue* value)
{
    return value->val._int;
}

JsonFloat json_value_get_float(const JsonValue* value)
{
    return value->val._float;
}

void json_value_destroy(JsonValue* value)
{
    if (value == NULL) return;
    switch (value->type) {
        case JTYPE_OBJECT:
            json_object_destroy(value->val._object);
            break;
        case JTYPE_ARRAY:
            json_array_destroy(value->val._array);
            break;
        case JTYPE_STRING:
            json_free(value->val._string);
            break;
        default:
            break;
    }
    json_free(value);
}

void json_value_print(const JsonValue* value)
{
    print_value(stdout, value, 0);
    puts("");
}

// ------------------- Array ------------------------

JsonArray* json_array_create(void)
{
    JsonArray* array = json_malloc(sizeof(JsonArray));
    array->values = NULL;
    array->num_values = 0;
    return array;
}

int json_array_length(const JsonArray* array)
{
    return array->num_values;
}

JsonValue* json_array_get(const JsonArray* array, int idx)
{
    return array->values[idx];
}

void json_array_append(JsonArray* array, JsonValue* value)
{
    array->values = push_value(array->values, value, &array->num_values);
}

void json_array_insert(JsonArray* array, int idx, JsonValue* value)
{
    JsonValue** new_values;
    int i;
    new_values = json_realloc(array->values,(array->num_values+1) * sizeof(JsonValue*));
    for (i = array->num_values; i > idx; i--)
        new_values[i] = new_values[i-1];
    new_values[idx] = value;
    array->values = new_values;
    array->num_values++;
}

void json_array_insert_fast(JsonArray* array, int idx, JsonValue* value)
{
    JsonValue** new_values;
    new_values = json_realloc(array->values,(array->num_values+1) * sizeof(JsonValue*));
    new_values[array->num_values] = new_values[idx];
    new_values[idx] = value;
    array->values = new_values;
    array->num_values++;
}

void json_array_remove(JsonArray* array, int idx)
{
    JsonValue** new_values;
    int i;
    json_value_destroy(array->values[idx]);
    for (i = idx; i < array->num_values-1; i++)
        array->values[i] = array->values[i+1];
    new_values = json_realloc(array->values,(array->num_values-1) * sizeof(JsonValue*));
    array->values = new_values;
    array->num_values--;
}

void json_array_remove_fast(JsonArray* array, int idx)
{
    JsonValue** new_values;
    json_value_destroy(array->values[idx]);
    array->values[idx] = array->values[array->num_values-1];
    new_values = json_realloc(array->values,(array->num_values-1) * sizeof(JsonValue*));
    array->values = new_values;
    array->num_values--;
}

void json_array_destroy(JsonArray* array)
{
    if (array == NULL) 
        return;
    json_values_destroy(array->values, array->num_values);
    json_free(array);
}

void json_array_print(const JsonArray* array)
{
    print_array(stdout, array, 0);
    puts("");
}

// ------------------- Iterator ------------------------

JsonIterator* json_iterator_create(const JsonObject* object)
{
    JsonIterator* iterator = json_malloc(sizeof(JsonIterator));
    if (iterator == NULL) {
        fprintf(stderr, "Failed to allocate memory for iterator");
        return NULL;
    }
    iterator->object = object;
    iterator->idx = 0;
    return iterator;
}

JsonMember* json_iterator_get(const JsonIterator* iterator)
{
    if (iterator->idx >= iterator->object->num_members)
        return NULL;
    return iterator->object->members[iterator->idx];
}

void json_iterator_increment(JsonIterator* iterator)
{
    iterator->idx++;
}

void json_iterator_destroy(JsonIterator* iterator)
{
    json_free(iterator);
}


// ---------------- Helper functions ------------------

static void json_members_destroy(JsonMember** members, int num_members)
{
    for (int i = 0; i < num_members; i++)
        json_member_destroy(members[i]);
    json_free(members);
}

static void json_values_destroy(JsonValue** values, int num_values)
{
    for (int i = 0; i < num_values; i++)
        json_value_destroy(values[i]);
    json_free(values);
}


static int json_member_cmp(const void* x, const void* y)
{
    const JsonMember* const* m1 = x;
    const JsonMember* const* m2 = y;
    return strcmp((*m1)->key, (*m2)->key);
}

static void* push_data(void** list, void* data, int* length, size_t size)
{
    void** new_list;
    if (list == NULL)
        new_list = json_malloc(size);
    else
        new_list = json_realloc(list, (*length+1) * size);
    if (new_list == NULL) {
        json_free(list);
        return NULL;
    }
    new_list[(*length)++] = data;
    return new_list;
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
    char* string;
    int n;
    if (fseek(file, start_pos, SEEK_SET) == -1) {
        print_error(line_num, "Something unexpected happened");
        return NULL;
    }
    n = end_pos - start_pos;
    string = json_malloc((n+1) * sizeof(char));
    if (string == NULL) {
        print_error(line_num, "Error allocating memory for string");
        return NULL;
    }
    if (fgets(string, n+1, file) == NULL) {
        json_free(string);
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
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
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
    long start_pos;
    int count, hex, dig;
    char* string;
    char c;
    c = get_next_nonspace(file, line_num);
    if (c != '"') {
        print_error(line_num, "Missing quotes");
        return NULL;
    }

    start_pos = ftell(file);
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

    string = json_malloc(count * sizeof(char));
    if (string == NULL) {
        print_error(line_num, "Error allocating memory for string");
        return NULL;
    }
    if (fseek(file, start_pos, SEEK_SET) == -1) {
        print_error(line_num, "Something unexpected happened");
        json_free(string);
        return NULL;
    }

    count = 0;
    while (1) {
        c = getch(file, line_num);
        if (c == '"') 
            break;
        if (c == EOF) {
            print_error(line_num, "EOF encountered while parsing string");
            json_free(string);
            return NULL;
        }
        if (c != '\\') {
            string[count++] = c;
            continue;
        }
        c = getch(file, line_num);
        if (c == EOF) {
            print_error(line_num, "EOF encountered while parsing string");
            json_free(string);
            return NULL;
        }
        if (c != 'u') {
            string[count++] = map_escape_sequence(c);
            continue;
        }
        dig = to_hex(c);
        if (dig == -1) {
            print_error(line_num, "Invalid hex literal");
            json_free(string);
            return NULL;
        }
        hex = dig << 12;
        for (int i = 2; i >= 0; i--) {
            c = getch(file, line_num);
            if (c == EOF) {
                print_error(line_num, "EOF encountered while parsing string");
                json_free(string);
                return NULL;
            }
            dig = to_hex(c);
            if (dig == -1) {
                print_error(line_num, "Invalid hex literal");
                json_free(string);
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
    JsonObject* object;
    JsonValue* value;

    object = parse_object(file, line_num);
    if (object == NULL) {
        print_error(line_num, "Error reading value object");
        return NULL;
    }
    value = json_malloc(sizeof(JsonValue));
    if (value == NULL) {
        print_error(line_num, "Error allocating memory for value");
        return NULL;
    }
    value->type = JTYPE_OBJECT;
    value->val._object = object;
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
        if (values == NULL) {
            print_error(line_num, "Error allocating memory for values");
            return NULL;
        }
    } while (peek_next_nonspace(file, line_num) != ']');

    return values;
}

static JsonArray* parse_array(FILE* file, int* line_num)
{
    JsonArray* array;
    JsonValue** values;
    int num_values;
    char c;
    c = get_next_nonspace(file, line_num);
    if (c == EOF) {
        print_error(line_num, "Expected '['");
        return NULL;
    }
    if (c != '[') {
        if (c == ']') {
            print_error(line_num, "Missing ']'");
        } else {
            print_error(line_num, "Unexpected character before '['");
        }
        return NULL;
    }
    
    c = peek_next_nonspace(file, line_num);
    if (c == ']') {
        getch(file, line_num);
        array = json_malloc(sizeof(JsonArray));
        if (array == NULL) {
            print_error(line_num, "Error allocating memory for array");
            return NULL;
        }
        array->values = NULL;
        array->num_values = 0;
        return array;
    }
 
    num_values = 0;
    values = parse_values(file, line_num, &num_values);

    if (values == NULL) {
        print_error(line_num, "Error parsing array");
        return NULL;
    }

    array = json_malloc(sizeof(JsonArray));
    if (array == NULL) {
        print_error(line_num, "Error allocating memory for array");
        json_values_destroy(values, num_values);
        return NULL;
    }
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
    JsonArray* array;
    JsonValue* value;
    array = parse_array(file, line_num);
    if (array == NULL) {
        print_error(line_num, "Error reading value array");
        return NULL;
    }
    value = json_malloc(sizeof(JsonValue));
    if (value == NULL) {
        print_error(line_num, "Error allocating memory for value in array");
        return NULL;
    }
    value->type = JTYPE_ARRAY;
    value->val._array = array;
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
    // consult dfa.png
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
    JsonValue* value;
    JsonType type;
    long start_pos, end_pos;
    double num;
    char* string;
    start_pos = ftell(file);
    if (start_pos == -1) {
        print_error(line_num, "Error reading file");
        return NULL;
    }
    end_pos = dfa_number(file, line_num, &type);
    if (end_pos == -1) {
        print_error(line_num, "Error reading file");
        return NULL;
    }
    if (end_pos == -2) {
        print_error(line_num, "Error parsing value num");
        return NULL;
    }

    string = get_string_in_range(file, line_num, start_pos, end_pos);

    if (string == NULL)
        return NULL;

    num = atof(string);
    json_free(string);

    value = json_malloc(sizeof(JsonValue));
    if (value == NULL) {
        print_error(line_num, "Error allocating memory for value");
        return NULL;
    }
    value->type = type;
    if (type == JTYPE_INT)
        value->val._int = num;
    else
        value->val._float = num;
    
    return value;
}

static JsonValue* parse_value_string(FILE* file, int* line_num)
{
    JsonValue* value;
    char* string;
    string = get_next_string(file, line_num);
    if (string == NULL) {
        print_error(line_num, "Error parsing value string");
        return NULL;
    }

    value = json_malloc(sizeof(JsonValue));
    if (value == NULL) {
        print_error(line_num, "Error allocating memory for value");
        return NULL;
    }
    value->type = JTYPE_STRING;
    value->val._string = string;

    return value;
}

static JsonValue* parse_value_true(FILE* file, int* line_num)
{
    JsonValue* value;
    char str[5];
    if (fgets(str, 5, file) == NULL) {
        print_error(line_num, "Error reading file");
        return NULL;
    }
    str[4] = '\0';
    if (strcmp(str, "true"))
        return NULL;
    value = json_malloc(sizeof(JsonValue));
    if (value == NULL) {
        print_error(line_num, "Error allocating memory for value");
        return NULL;
    }
    value->type = JTYPE_TRUE;
    return value;
}

static JsonValue* parse_value_false(FILE* file, int* line_num)
{
    JsonValue* value;
    char str[6];
    if (fgets(str, 6, file) == NULL) {
        print_error(line_num, "Error reading file");
        return NULL;
    }
    str[5] = '\0';
    if (strcmp(str, "false"))
        return NULL;
    value = json_malloc(sizeof(JsonValue));
    if (value == NULL) {
        print_error(line_num, "Error allocating memory for value");
        return NULL;
    }
    value->type = JTYPE_FALSE;
    return value;
}

static JsonValue* parse_value_null(FILE* file, int* line_num)
{
    JsonValue* value;
    char str[5];
    if (fgets(str, 5, file) == NULL) {
        print_error(line_num, "Error reading file");
        return NULL;
    }
    str[4] = '\0';
    if (strcmp(str, "null"))
        return NULL;
    value = json_malloc(sizeof(JsonValue));
    if (value == NULL) {
        print_error(line_num, "Error allocating memory for value");
        return NULL;
    }
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
    JsonValue* value;
    JsonMember* member;
    char* key;
    char c;
    key = get_next_string(file, line_num);
    if (key == NULL) {
        print_error(line_num, "Error reading key");
        return NULL;
    }

    c = get_next_nonspace(file, line_num);
    if (c != ':') {
        print_error(line_num, "Missing colon");
        json_free(key);
        return NULL;
    }

    value = parse_value(file, line_num);
    if (value == NULL) {
        print_error(line_num, "Error reading value");
        json_free(key);
        return NULL;
    }

    member = json_malloc(sizeof(JsonMember));
    if (member == NULL) {
        print_error(line_num, "Error allocating memory for member");
        json_value_destroy(value);
        json_free(key);
        return NULL;
    }
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
        if (members == NULL) {
            print_error(line_num, "Error allocating memory for members");
            return NULL;
        }
    } while (peek_next_nonspace(file, line_num) != '}');

    qsort(members, *num_members, sizeof(JsonMember*), json_member_cmp); 

    return members;
}

static JsonObject* parse_object(FILE* file, int* line_num)
{
    JsonObject* object;
    JsonMember** members;
    int num_members;
    char c;

    c = get_next_nonspace(file, line_num);
    if (c == EOF) {
        print_error(line_num, "Expected '{'");
        return NULL;
    }
    if (c != '{') {
        if (c == '}') {
            print_error(line_num, "Missing '{'");
        } else {
            print_error(line_num, "Unexpected character before '{'");
        }
        return NULL;
    }

    c = peek_next_nonspace(file, line_num);
    if (c == '}') {
        getch(file, line_num);
        object = json_malloc(sizeof(JsonObject));
        if (object == NULL) {
            print_error(line_num, "Error allocating memory for member");
            return NULL;
        }
        object->members = NULL;
        object->num_members = 0;
        return object;
    }

    num_members = 0;
    members = parse_members(file, line_num, &num_members);

    if (members == NULL) {
        print_error(line_num, "Error parsing object");
        return NULL;
    }

    object = json_malloc(sizeof(JsonObject));
    if (object == NULL) {
        print_error(line_num, "Error parsing object");
        json_members_destroy(members, num_members);
        return NULL;
    }
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

static void tab(FILE* fptr, int depth)
{
    for (int j = 0; j < depth; j++)
        fprintf(fptr, "  ");
}

static void print_string(FILE* fptr, char* string)
{
    fprintf(fptr, "\"");
    for (int i = 0; string[i] != '\0'; i++) {
        if (string[i] == '"' || string[i] == '\\' || string[i] == '/')
            fprintf(fptr, "\\");
        fprintf(fptr, "%c", string[i]);
    }
    fprintf(fptr, "\"");
}

static void print_value(FILE* fptr, const JsonValue* value, int depth)
{
    switch (value->type) {
        case JTYPE_TRUE:
            fprintf(fptr, "true");
            break;
        case JTYPE_FALSE:
            fprintf(fptr, "false");
            break;
        case JTYPE_NULL:
            fprintf(fptr, "null");
            break;
        case JTYPE_STRING:
            print_string(fptr, value->val._string);
            break;
        case JTYPE_INT:
            fprintf(fptr, "%lld", value->val._int);
            break;
        case JTYPE_FLOAT:
            fprintf(fptr, "%f", value->val._float);
            break;
        case JTYPE_OBJECT:
            print_object(fptr, value->val._object, depth);
            break;
        case JTYPE_ARRAY:
            print_array(fptr, value->val._array, depth);
            break;
        default:
            break;
    }
}

static void print_array(FILE* fptr, const JsonArray* array, int depth)
{
    const JsonValue* value;

    fprintf(fptr, "[");
    if (array->num_values == 0) {
        fprintf(fptr, "]");
        return;
    }
    fprintf(fptr, "\n");

    value = array->values[0];
    tab(fptr, depth+1);
    print_value(fptr, value, depth+1);

    for (int i = 1; i < array->num_values; i++) {
        value = array->values[i];
        fprintf(fptr, ",\n");
        tab(fptr, depth+1);
        print_value(fptr, value, depth+1);
    }
    fprintf(fptr, "\n");
    tab(fptr, depth);
    fprintf(fptr, "]");
}

static void print_object(FILE* fptr, const JsonObject* object, int depth)
{
    const JsonMember* member;
    
    fprintf(fptr, "{");
    if (object->num_members == 0) {
        fprintf(fptr, "}");
        return;
    }
    fprintf(fptr, "\n");

    member = object->members[0];
    tab(fptr, depth+1);
    fprintf(fptr, "\"%s\": ", member->key);
    print_value(fptr, member->value, depth+1);

    for (int i = 1; i < object->num_members; i++) {
        member = object->members[i];
        fprintf(fptr, ",\n");
        tab(fptr, depth+1);
        fprintf(fptr, "\"%s\": ", member->key);
        print_value(fptr, member->value, depth+1);
    }
    fprintf(fptr, "\n");
    tab(fptr, depth);
    fprintf(fptr, "}");
}

