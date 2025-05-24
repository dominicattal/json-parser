#ifndef JSON_H
#define JSON_h

typedef enum {
    JTYPE_OBJECT,
    JTYPE_ARRAY,
    JTYPE_STRING,
    JTYPE_INT,
    JTYPE_FLOAT,
    JTYPE_TRUE,
    JTYPE_FALSE,
    JTYPE_NULL
} JsonType;

typedef struct JsonValue JsonValue;
typedef struct JsonObject JsonObject;
typedef struct JsonArray JsonArray;
typedef struct JsonMember JsonMember;

// reads json file. returns NULL on error
JsonObject* json_read(const char* path);

// json_get_value returns NULL if key does not exist in object
JsonValue*  json_get_value(const JsonObject* object, const char* key);
JsonType    json_get_type(const JsonValue* value);
JsonObject* json_get_object(const JsonValue* value);
JsonArray*  json_get_array(const JsonValue* value);
char*       json_get_string(const JsonValue* value);
int         json_get_int(const JsonValue* value);
float       json_get_float(const JsonValue* value);

// returns the number of values in the array. Undefined if array is NULL
int json_array_length(const JsonArray* array);

// index into array. Undefined if idx is out of 
// bounds of the array or if array is NULL
JsonValue* json_array_get(const JsonArray* array, int idx);

// print out json objects, array, and values, formatted like json
void json_print_object(const JsonObject* object);
void json_print_array(const JsonArray* array);
void json_print_value(const JsonValue* value);

// only works with json object returned from json_read
// json_object_destroy does nothing if target is NULL
void json_object_destroy(JsonObject* object);

#endif
