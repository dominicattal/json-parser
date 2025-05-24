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
    JsonValue* value = json_get_value(root_object, "key1");
    assert(value != NULL);
    assert(json_get_type(value) == JTYPE_ARRAY);
    JsonArray* array = json_get_array(value);
    for (int i = 0; i < json_array_length(array); i++) {
        value = json_array_get(array, i); 
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

int main(int argc, char** argv)
{
    if (argc > 1)
        test_dir(argv[1]);
    test_array();
    return 0;
}
