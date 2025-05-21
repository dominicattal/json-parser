#ifndef JSON_H
#define JSON_h

typedef enum {
    JTYPE_OBJECT,
    JTYPE_ARRAY,
    JTYPE_STRING,
    JTYPE_NUMBER,
    JTYPE_TRUE,
    JTYPE_FALSE,
    JTYPE_NULL
} JsonType;

typedef struct JsonValue JsonValue;
typedef struct JsonObject JsonObject;
typedef struct JsonArray JsonArray;
typedef struct JsonArrayIterator JsonArrayIterator;

// reads json file. returns NULL on error
JsonObject* json_read(const char* path);

// returns 1 if key exists, 0 otherwise
int json_key_exists(const JsonObject* object, const char* key);

// returns JsonType
JsonType json_get_type(const JsonObject* object);

// Gets are undefined for nonexistent keys or values of wrong type
// call json_key_exists and json_get_type first
// returns json object at key
const JsonObject* json_get(const JsonObject* object, const char* key);
const JsonArray* json_get_array(const JsonObject* object, const char* key);
const char* json_get_string(const JsonObject* object, const char* key);
int json_get_int(const JsonObject* object, const char* key);
float json_get_float(const JsonObject* object, const char* key);

JsonArrayIterator* json_get_array_iterator(JsonArray* array);
JsonObject* json_iterator_value(JsonArrayIterator* iterator);
void json_iterator_next(JsonArrayIterator* iterator);
int  json_iterator_end(JsonArrayIterator* iterator);
void json_destroy_array_iterator(JsonArrayIterator* iterator);

// destroy json object. Does nothing if object is NULL
void json_object_destroy(JsonObject* object);

#endif
