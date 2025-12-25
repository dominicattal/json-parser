#include "json.h"
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
    root_object = json_read("positives/array_val_4.json");
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
    root_object = json_read("positives/number.json");
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
    JsonObject* object1 = json_read("positives/number.json");
    JsonObject* object2 = json_read("positives/array_val.json");
    JsonObject* object3 = json_merge_objects(object1, object2);
    json_object_print(object3);
    json_object_destroy(object3);
    //json_object_destroy(object2);
}

int main(int argc, char** argv)
{
    if (argc > 1)
        test_dir(argv[1]);
    test_array();
    puts("------------------------");
    test_iterator();
    puts("------------------------");
    test_merge();
    return 0;
}
