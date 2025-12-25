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

typedef struct JsonObject JsonObject;
typedef struct JsonMember JsonMember;
typedef struct JsonValue JsonValue;
typedef struct JsonArray JsonArray;
typedef struct JsonIterator JsonIterator;

/*
Summary of api, more info below
---------- Objects ------------
JsonObject*     json_read(const char* path);
JsonObject*     json_object_create(void);
JsonObject*     json_merge_objects(JsonObject* object1, JsonObject* object2);
int             json_object_length(const JsonObject* object);
JsonValue*      json_object_get_value(const JsonObject* object, const char* key);
void            json_object_attach(JsonObject* object, JsonMember* member);
void            json_object_detach(JsonObject* object, JsonMember* member);
JsonMember*     json_object_detach_by_key(JsonObject* object, const char* key);
void            json_object_destroy(JsonObject* object);
void            json_object_print(JsonObject* object);

----------- Members ------------
JsonMember*     json_member_create(const char* key, const JsonValue* value);
char*           json_member_key(const JsonMember* member);
JsonValue*      json_member_value(const JsonMember* member);
void            json_member_update(JsonMember* member, JsonValue* value);
void            json_member_destroy(JsonMember* member);
void            json_member_print(JsonMember* member);

----------- Value --------------
JsonValue*      json_value_create(JsonType type, void* data);
JsonType        json_value_get_type(const JsonValue* value);
JsonObject*     json_value_get_object(const JsonValue* value);
JsonArray*      json_value_get_array(const JsonValue* value);
char*           json_value_get_string(const JsonValue* value);
long long       json_value_get_int(const JsonValue* value);
double          json_value_get_float(const JsonValue* value);
void            json_value_update(JsonValue* value, JsonType type, void* data);
void            json_value_destroy(JsonValue* value);
void            json_value_print(const JsonValue* value);

----------- Array ----------------
JsonArray*      json_array_create(void);
int             json_array_length(const JsonArray* array);
JsonValue*      json_array_get(const JsonArray* array, int idx);
void            json_array_append(JsonArray* array, JsonValue* value);
void            json_array_insert(JsonArray* array, int idx, JsonValue* value);
void            json_array_insert_fast(JsonArray* array, int idx, JsonValue* value);
void            json_array_destroy(JsonArray* array);
void            json_array_print(const JsonArray* array);

--------- Iterator ----------------
JsonIterator*   json_iterator_create(const JsonObject* object);
JsonMember*     json_iterator_get(const JsonIterator* iterator);
void            json_iterator_increment(JsonIterator* iterator);
void            json_iterator_destroy(JsonIterator* iterator);

-----------------------------------
*/

// reads json file. returns NULL on error
JsonObject*     json_read(const char* path);

// Create empty json object
JsonObject*     json_object_create(void);

// creates a new json object from two objects. the new object is also in 
// sorted order. On success, object1 and object2 are destroyed in the process, so do
// not call json_object_destroy on them. Returns NULL on failure, in which case
// you must call json_object_destroy on the two objects. Undefined if object1
// or object2 are NULL.
JsonObject*     json_merge_objects(JsonObject* object1, JsonObject* object2);

// returns the number of members in an object. Undefined if object is NULL
int             json_object_length(const JsonObject* object);

// json_get_value returns NULL if key does not exist in object
JsonValue*      json_object_get_value(const JsonObject* object, const char* key);

void            json_object_attach(JsonObject* object, JsonMember* member);
void            json_object_detach(JsonObject* object, JsonMember* member);
JsonMember*     json_object_detach_by_key(JsonObject* object, const char* key);

// only works with json object returned from json_read
// or json_merge_objects. json_object_destroy does nothing if target is NULL
void            json_object_destroy(JsonObject* object);

// Adds to existing json object
JsonMember*     json_member_create(const char* key, const JsonValue* value);
void            json_member_update(JsonMember* member, JsonValue* value);
void            json_member_destroy(JsonMember* member);

// extract key value pair from member. Undefined if member is NULL
char*           json_member_key(const JsonMember* member);
JsonValue*      json_member_value(const JsonMember* member);

// Creates value from data. The value of data is derefenced based on the supplied type
// and stored in a new JsonValue. The memory allocated for data is managed by the JsonValue
// once you supply it, so you should not free it and instead call json_value_destroy
// JTYPE_OBJECT -> JsonObject*
// JTYPE_ARRAY  -> JsonArray*
// JTYPE_STRING -> char*
// JTYPE_INT    -> long long
// JTYPE_FLOAT  -> double
// JTYPE_TRUE   -> ignored, can pass NULL
// JTYPE_FALSE  -> ignored, can pass NULL
JsonValue*      json_value_create(JsonType type, void* data);
void            json_value_update(JsonValue* value, JsonType type, void* data);
void            json_value_destroy(JsonValue* value);

// returns the type of a value for use with a getter function
JsonType        json_value_get_type(const JsonValue* value);

// getter functions have undefined behavior if their type does
// not match the function
JsonObject*     json_value_get_object(const JsonValue* value);
JsonArray*      json_value_get_array(const JsonValue* value);
char*           json_value_get_string(const JsonValue* value);
long long       json_value_get_int(const JsonValue* value);
double          json_value_get_float(const JsonValue* value);

// Creates an empty array. 
JsonArray*      json_array_create(void);
void            json_array_append(JsonArray* array, JsonValue* value);
void            json_array_insert(JsonArray* array, int idx, JsonValue* value);
void            json_array_insert_fast(JsonArray* array, int idx, JsonValue* value);

// returns the number of values in the array. Undefined if array is NULL
int             json_array_length(const JsonArray* array);

// index into array. Undefined if idx is out of 
// bounds of the array or if array is NULL
JsonValue*      json_array_get(const JsonArray* array, int idx);
void            json_array_destroy(JsonArray* array);

// creates an iterator for traversing through key-value pairs in an object.
JsonIterator*   json_iterator_create(const JsonObject* object);

// gets the key-value pair that the iterator points to
// returns NULL if the iterator is at the last pair
JsonMember*     json_iterator_get(const JsonIterator* iterator);

// moves to the next key-value pair. Undefined if iterator is NULL
void            json_iterator_increment(JsonIterator* iterator);

// destroys an iterator. Does nothing if iterator is NULL
void            json_iterator_destroy(JsonIterator* iterator);

// print out json objects, array, and values, formatted like json
// undefined if object is null
void            json_object_print(const JsonObject* object);
void            json_array_print(const JsonArray* array);
void            json_value_print(const JsonValue* value);

#endif
