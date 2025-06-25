#include "json.h"
#include <stdio.h>
#include <dirent.h>
#include <assert.h>

static void test_dir(const char* path)
{
    char str[512];
    DIR* tests = opendir(path);
    assert(tests != NULL);
    struct dirent* entry;
    while ((entry = readdir(tests)) != NULL) {
        if (!strcmp(entry->d_name, "."))
            continue;
        if (!strcmp(entry->d_name, ".."))
            continue;
        sprintf(str, "%s/%s", path, entry->d_name);
        puts(str);
        puts("");
        JsonObject* object = json_read(str);
        if (object != NULL)
            json_print_object(object);
        else
            puts("Parsing failed");
        json_object_destroy(object);
        puts("-------------------------");
    }
}

static void test_array(void)
{
    JsonObject* root_object = json_read("positives/array_val_4.json");
    assert(root_object != NULL);
    JsonValue* value = json_get_value(root_object, "key1");
    assert(value != NULL);
    assert(json_get_type(value) == JTYPE_ARRAY);
    JsonArray* array = json_get_array(value);
    for (int i = 0; i < json_array_length(array); i++) {
        value = json_array_get(array, i); 
        assert(value != NULL);
        json_print_value(value);
        if (json_get_type(value) == JTYPE_OBJECT) {
            JsonObject* object = json_get_object(value);
            value = json_get_value(object, "key2");
            assert(value != NULL);
            assert(json_get_type(value) == JTYPE_STRING);
            char* string = json_get_string(value);
            puts(string);
        }
    }
    json_object_destroy(root_object);
}

static void test_iterator(void)
{
    JsonObject* root_object = json_read("positives/number.json");
    assert(root_object != NULL);
    JsonIterator* it = json_iterator_create(root_object);
    while (json_iterator_get(it) != NULL) {
        JsonMember* member = json_iterator_get(it);
        char* key = json_member_key(member);
        JsonValue* val = json_member_value(member);
        puts(key);
        json_print_value(val);
        json_iterator_increment(it);
    }
    json_object_destroy(root_object);
}

static void test_merge(void)
{
    JsonObject* object1 = json_read("positives/number.json");
    JsonObject* object2 = json_read("positives/array_val.json");
    JsonObject* object3 = json_merge_objects(object1, object2);
    json_print_object(object3);
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
