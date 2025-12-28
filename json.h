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

typedef struct JsonObject   JsonObject;
typedef struct JsonMember   JsonMember;
typedef struct JsonValue    JsonValue;
typedef struct JsonArray    JsonArray;
typedef struct JsonIterator JsonIterator;
typedef long long           JsonInt;
typedef double              JsonFloat;

/*
Summary of api, more info below
---------- Objects ------------
JsonObject*     json_read(const char* path);
JsonObject*     json_object_create(void);
JsonObject*     json_merge_objects(JsonObject* object1, JsonObject* object2);
int             json_object_length(const JsonObject* object);
JsonMember*     json_object_get_member(const JsonObject* object, const char* key);
JsonValue*      json_object_get_value(const JsonObject* object, const char* key);
int             json_object_attach(JsonObject* object, JsonMember* member);
JsonMember*     json_object_detach(JsonObject* object, const char* key);
void            json_object_detach_and_destroy(JsonObject* object, const char* key);
void            json_object_destroy(JsonObject* object);
void            json_object_print(JsonObject* object);
int             json_object_write(const JsonObject* object, const char* filename);

----------- Members ------------
JsonMember*     json_member_create(const char* key, JsonValue* value);
char*           json_member_get_key(const JsonMember* member);
JsonValue*      json_member_get_value(const JsonMember* member);
void            json_member_update(JsonMember* member, JsonValue* value);
void            json_member_destroy(JsonMember* member);
void            json_member_print(JsonMember* member);

----------- Value --------------
JsonValue*      json_value_create_null(void);
JsonValue*      json_value_create_true(void);
JsonValue*      json_value_create_false(void);
JsonValue*      json_value_create_int(JsonInt val);
JsonValue*      json_value_create_float(JsonFlaot val);
JsonValue*      json_value_create_string(char* val);
JsonValue*      json_value_create_object(JsonObject* val);
JsonValue*      json_value_create_array(JsonArray* val);
JsonType        json_value_get_type(const JsonValue* value);
JsonObject*     json_value_get_object(const JsonValue* value);
JsonArray*      json_value_get_array(const JsonValue* value);
char*           json_value_get_string(const JsonValue* value);
JsonInt         json_value_get_int(const JsonValue* value);
JsonFloat       json_value_get_float(const JsonValue* value);
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
void            json_array_remove(JsonArray* array, int idx);
void            json_array_remove_fast(JsonArray* array, int idx);
void            json_array_destroy(JsonArray* array);
void            json_array_print(const JsonArray* array);

--------- Iterator ----------------
JsonIterator*   json_iterator_create(const JsonObject* object);
JsonMember*     json_iterator_get(const JsonIterator* iterator);
void            json_iterator_increment(JsonIterator* iterator);
void            json_iterator_destroy(JsonIterator* iterator);

-----------------------------------
*/

// reads json file. returns NULL on error. Can only read json objects and not json arrays.
JsonObject*     json_read(const char* path);

// Create empty json object. Returns NULL if allocation fails.
JsonObject*     json_object_create(void);

// creates a new json object from two objects. the new object is also in 
// sorted order. On success, object1 and object2 are destroyed in the process, so do
// not call json_object_destroy on them. Returns NULL on failure, in which case
// you must call json_object_destroy on the two objects. Undefined if object1
// or object2 are NULL.
JsonObject*     json_merge_objects(JsonObject* object1, JsonObject* object2);

// returns the number of members in an object. Undefined if object is NULL
int             json_object_length(const JsonObject* object);

// returns NULL if key does not exist in object. Undefined if object is NULL
JsonMember*     json_object_get_member(const JsonObject* object, const char* key);

// returns NULL if key does not exist in object. Undefinedd if object is NULL.
JsonValue*      json_object_get_value(const JsonObject* object, const char* key);

// attach a member to an object. the object owns the member once the member is attached.
// returns nonzero if member with key already exists in object or if allocation fails
int             json_object_attach(JsonObject* object, JsonMember* member);

// search for a member in the object based on the given key, detach it, and return it.
// returns NULL if no member is found with that key.
JsonMember*     json_object_detach(JsonObject* object, const char* key);

// search for a member in the object, detach it, and destroy it. does nothing if no
// member is found with that key
void            json_object_detach_and_destroy(JsonObject* object, const char* key);

// only works with json object returned from json_read
// or json_merge_objects. json_object_destroy does nothing if target is NULL
void            json_object_destroy(JsonObject* object);

// print json object. Undefined if object is NULL.
void            json_object_print(const JsonObject* object);

// writes json object to file. Returns nonzero on failure, zero on success
int             json_object_write(const JsonObject* object, const char* filename);

// creates a new json member to add to a json object. A copy of the string key is made, so
// you are responsible for the original pointer. The member takes control of the value,
// so you do not need to destroy it.
JsonMember*     json_member_create(const char* key, JsonValue* value);

// replaces the value in member with value. the old value in member is destroyed.
void            json_member_update(JsonMember* member, JsonValue* value);

// destroys a member. must not be attached to object, or everything will break.
void            json_member_destroy(JsonMember* member);

// prints json member
void            json_member_print(JsonMember* member);

// extract key value pair from member. Undefined if member is NULL
char*           json_member_get_key(const JsonMember* member);
JsonValue*      json_member_get_value(const JsonMember* member);

// Creates value from data. The memory allocated for data is managed by the JsonValue
// once you supply it, so you should not free it and instead call json_value_destroy
JsonValue*      json_value_create_null(void);
JsonValue*      json_value_create_true(void);
JsonValue*      json_value_create_false(void);
JsonValue*      json_value_create_int(JsonInt val);
JsonValue*      json_value_create_float(JsonFloat val);
JsonValue*      json_value_create_string(char* val);
JsonValue*      json_value_create_object(JsonObject* val);
JsonValue*      json_value_create_array(JsonArray* val);
void            json_value_destroy(JsonValue* value);

// returns the type of a value for use with a getter function
JsonType        json_value_get_type(const JsonValue* value);

// getter functions have undefined behavior if their type does not match the function
JsonObject*     json_value_get_object(const JsonValue* value);
JsonArray*      json_value_get_array(const JsonValue* value);
char*           json_value_get_string(const JsonValue* value);
long long       json_value_get_int(const JsonValue* value);
double          json_value_get_float(const JsonValue* value);

// prints value. undefined if value is NULL
void            json_value_print(const JsonValue* value);

// Creates an empty array. 
JsonArray*      json_array_create(void);

// Appends the value to the end of the array.
void            json_array_append(JsonArray* array, JsonValue* value);

// Inserts the value into the array at the specified idx in O(n) and preserves order
void            json_array_insert(JsonArray* array, int idx, JsonValue* value);

// Inserts the value into the array at the specified idx in O(1) but doesn't preserve order
void            json_array_insert_fast(JsonArray* array, int idx, JsonValue* value);

// Removes and destroys the value at idx in O(n) and preserves order
// Undefined if idx is out of bounds or if memory fails to reallocate
void            json_array_remove(JsonArray* array, int idx);

// Removes and destroys the value at idx in O(1) but doesn't preserves order
// Undefined if array is NULL, if idx is out of bounds, or if memory fails to reallocate
void            json_array_remove_fast(JsonArray* array, int idx);

// returns the number of values in the array. Undefined if array is NULL
int             json_array_length(const JsonArray* array);

// index into array. Undefined if idx is out of bounds of the array or if array is NULL
JsonValue*      json_array_get(const JsonArray* array, int idx);

// Destroys json array and all its JsonValues. Does nothing if array is NULL.
void            json_array_destroy(JsonArray* array);

// Prints json array. Undefined if array is NULL.
void            json_array_print(const JsonArray* array);

// creates an iterator for traversing through key-value pairs in an object.
JsonIterator*   json_iterator_create(const JsonObject* object);

// gets the key-value pair that the iterator points to
// returns NULL if the iterator is at the last pair
JsonMember*     json_iterator_get(const JsonIterator* iterator);

// moves to the next key-value pair. Undefined if iterator is NULL
void            json_iterator_increment(JsonIterator* iterator);

// destroys an iterator. Does nothing if iterator is NULL
void            json_iterator_destroy(JsonIterator* iterator);

#endif
