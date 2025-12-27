#include "json.h"
#define _GNU_SOURCE // asprintf
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <assert.h>

static void test_dir(const char* path)
{
    JsonObject* object;
    struct dirent* entry;
    char str[512];
    DIR* tests = opendir(path);
    assert(tests != NULL);
    while ((entry = readdir(tests)) != NULL) {
        if (!strcmp(entry->d_name, "."))
            continue;
        if (!strcmp(entry->d_name, ".."))
            continue;
        sprintf(str, "%s/%s", path, entry->d_name);
        puts(str);
        puts("");
        object = json_read(str);
        if (object != NULL)
            json_object_print(object);
        else
            puts("Parsing failed");
        json_object_destroy(object);
        puts("-------------------------");
    }
}

static void test_array(void)
{
    JsonObject* root_object;
    JsonObject* object;
    JsonValue* value;
    JsonArray* array;
    char* string;
    root_object = json_read("tests/positives/array_val_4.json");
    assert(root_object != NULL);
    value = json_object_get_value(root_object, "key1");
    assert(value != NULL);
    assert(json_value_get_type(value) == JTYPE_ARRAY);
    array = json_value_get_array(value);
    for (int i = 0; i < json_array_length(array); i++) {
        value = json_array_get(array, i); 
        assert(value != NULL);
        json_value_print(value);
        if (json_value_get_type(value) == JTYPE_OBJECT) {
            object = json_value_get_object(value);
            value = json_object_get_value(object, "key2");
            assert(value != NULL);
            assert(json_value_get_type(value) == JTYPE_STRING);
            string = json_value_get_string(value);
            puts(string);
        }
    }
    json_object_destroy(root_object);
}

static void test_iterator(void)
{
    JsonObject* root_object;
    JsonMember* member;
    JsonValue* value;
    JsonIterator* it;
    char* key;
    root_object = json_read("tests/positives/number.json");
    assert(root_object != NULL);
    it = json_iterator_create(root_object);
    while (json_iterator_get(it) != NULL) {
        member = json_iterator_get(it);
        key = json_member_key(member);
        value = json_member_value(member);
        puts(key);
        json_value_print(value);
        json_iterator_increment(it);
    }
    json_object_destroy(root_object);
    json_iterator_destroy(it);
}

static void test_merge(void)
{
    JsonObject* object1 = json_read("tests/positives/number.json");
    JsonObject* object2 = json_read("tests/positives/array_val.json");
    JsonObject* object3 = json_merge_objects(object1, object2);
    json_object_print(object3);
    json_object_destroy(object3);
}

static void test_crud(void)
{
    JsonObject* object;
    JsonObject* obj2;
    JsonMember* member;
    JsonValue* value;
    JsonArray* array;
    JsonInt integer;
    JsonFloat decimal;
    char* string;

    object = json_object_create();
    puts("Empty object");
    json_object_print(object);
    puts("--------------------");
    puts("Insert some stuff");
    value = json_value_create_null();
    member = json_member_create("key_null", value);
    json_object_attach(object, member);
    value = json_value_create_false();
    member = json_member_create("key_false", value);
    json_object_attach(object, member);
    value = json_value_create_true();
    member = json_member_create("key_true", value);
    json_object_attach(object, member);
    integer = 67;
    value = json_value_create_int(integer);
    member = json_member_create("key_int", value);
    json_object_attach(object, member);
    decimal = 6.7;
    value = json_value_create_float(decimal);
    member = json_member_create("key_float", value);
    json_object_attach(object, member);
    obj2 = json_object_create();
    value = json_value_create_object(obj2);
    member = json_member_create("key_obj", value);
    json_object_attach(object, member);
    integer = 7;
    value = json_value_create_int(integer);
    member = json_member_create("6", value);
    json_object_attach(obj2, member);
    asprintf(&string, "This is a value");
    value = json_value_create_string(string);
    member = json_member_create("key_string", value);
    json_object_attach(object, member);
    array = json_array_create();
    value = json_value_create_array(array);
    member = json_member_create("key_empty_array", value);
    json_object_attach(object, member);
    json_object_print(object);
    puts("--------------------");
    puts("Initial Array");
    array = json_array_create();
    value = json_value_create_null();
    json_array_append(array, value);
    value = json_value_create_true();
    json_array_append(array, value);
    value = json_value_create_false();
    json_array_append(array, value);
    json_array_print(array);
    puts("--------------------");
    puts("Insert value");
    integer = 69;
    value = json_value_create_int(integer);
    json_array_insert(array, 1, value);
    json_array_print(array);
    puts("--------------------");
    puts("Insert value fast");
    decimal = 6.9;
    value = json_value_create_float(decimal);
    json_array_insert_fast(array, 1, value);
    json_array_print(array);
    puts("--------------------");
    puts("Remove value");
    json_array_remove(array, 2);
    json_array_print(array);
    puts("--------------------");
    puts("Remove value fast");
    json_array_remove_fast(array, 0);
    json_array_print(array);
    puts("--------------------");
    puts("Everything");
    value = json_value_create_array(array);
    member = json_member_create("key_array", value);
    json_object_attach(object, member);

    json_object_print(object);

    json_object_destroy(object);
}

int main(int argc, char** argv)
{
    if (argc > 1)
        test_dir(argv[1]);
    //test_array();
    puts("------------------------");
    //test_iterator();
    puts("------------------------");
    //test_merge();
    puts("------------------------");
    test_crud();
    return 0;
}
