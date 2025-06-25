#ifndef JSON_H
#define JSON_H

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
typedef struct JsonIterator JsonIterator;

// reads json file. returns NULL on error
JsonObject* json_read(const char* path);

// only works with json object returned from json_read
// or json_merge_objects. json_object_destroy does nothing if target is NULL
void json_object_destroy(JsonObject* object);

// json_get_value returns NULL if key does not exist in object
JsonValue* json_get_value(const JsonObject* object, const char* key);

// returns the type of a value for use with a getter function
JsonType json_get_type(const JsonValue* value);

// getter functions have undefined behavior if their type does
// not match the function
JsonObject* json_get_object(const JsonValue* value);
JsonArray*  json_get_array(const JsonValue* value);
char*       json_get_string(const JsonValue* value);
long long   json_get_int(const JsonValue* value);
double      json_get_float(const JsonValue* value);

// extract key value pair from member. Undefined if member is NULL
char* json_member_key(const JsonMember* member);
JsonValue* json_member_value(const JsonMember* member);

// returns the number of members in an object. Undefined if object is NULL
int json_object_length(const JsonObject* object);

// creates a new json object from two objects. the new object is also in 
// sorted order. On success, object1 and object2 are destroyed in the process, so do
// not call json_object_destroy on them. Returns NULL on failure, in which case
// you must call json_object_destroy on the two objects. Undefined if object1
// or object2 are NULL.
JsonObject* json_merge_objects(JsonObject* object1, JsonObject* object2);

// creates an iterator for traversing through key-value pairs
// in an object. Because of how pairs are stored, the iterator
// will move through the keys in sorted order. Undefined if
// object is NULL. Returns NULL on failure
JsonIterator* json_iterator_create(const JsonObject* object);

// gets the key-value pair that the iterator points to
// returns NULL if the iterator is at the last pair
JsonMember* json_iterator_get(const JsonIterator* iterator);

// moves to the next key-value pair. Undefined if iterator is NULL
void json_iterator_increment(JsonIterator* iterator);

// destroys an iterator. Does nothing if iterator is NULL
void json_iterator_destroy(JsonIterator* iterator);

// returns the number of values in the array. Undefined if array is NULL
int json_array_length(const JsonArray* array);

// index into array. Undefined if idx is out of 
// bounds of the array or if array is NULL
JsonValue* json_array_get(const JsonArray* array, int idx);

// print out json objects, array, and values, formatted like json
// undefined if object is null
void json_print_object(const JsonObject* object);
void json_print_array(const JsonArray* array);
void json_print_value(const JsonValue* value);

#endif
