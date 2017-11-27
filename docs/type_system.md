# Confluo Type System

Confluo uses a strictly typed system. While primitive data types like
`BOOL`, `CHAR`, `SHORT`, `INT`, `LONG`, `FLOAT`, `DOUBLE` and `STRING`
are supported by default in Confluo, it is possible to add custom 
user-defined data types. This requires defining a few operations that would
allow operations like applying filters and triggers on attributes of
the custom data type.

To create a new type, we need to define the following properties so that
native operations can be supported; these properties are summarised in the
[`type_properties`](../libconfluo/confluo/types/type_properties.h) 
struct:

```cpp
struct type_properties {
  std::string name;
  size_t size;

  void* min;
  void* max;
  void* one;
  void* zero;

  bool is_numeric;

  rel_ops_t relational_ops;
  unary_ops_t unary_ops;
  binary_ops_t binary_ops;
  key_op_t key_transform_op;

  parse_op_t parse_op;
  to_string_op_t to_string_op;

  serialize_op_t serialize_op;
  deserialize_op_t deserialize_op;

  ...
}
```

* `std::string name` - A unique name for the type
* `size_t size` -  The size of underlying representation for fixed sized types. This should be set to zero for 
dynamically sized types (e.g., see definintion for [`STRING`](../libconfluo/confluo/types/type_properties.h) type).
* `void* min` - This is a pointer to the minimum value that the type
can hold. See [type_properties.h](../libconfluo/confluo/types/type_properties.h) 
to see examples of `min` assigned to primitive types.
* `void* max` - This is a pointer to the maximum value that the type
can hold. See [type_properties.h](../libconfluo/confluo/types/type_properties.h) 
to see examples of `max` assigned to primitive types.
* `void* one` - This is a pointer to the step value with which the type
can be incremented. See [type_properties.h](../libconfluo/confluo/types/type_properties.h) 
to see examples of `one` assigned to primitive types.
* `void* zero` - This is a pointer to the zero value for the type. See 
[type_properties.h](../libconfluo/confluo/types/type_properties.h) to see 
examples of `zero` assigned to primitive types.
* `bool is_numeric` - This indicates whether the type is numeric or not; 
numeric types typically support most arithmetic operators; see 
[arithmetic_ops.h][../libconfluo/confluo/types/arithmetic_ops.h] for examples.
* `relational_ops_t relational_ops` - Stores a list of relational operator functions
for the given type, so that operations like `filter` can work. See 
[rel_ops.h](../libconfluo/confluo/types/relational_ops.h) for examples of
what relational functions can be defined.
* `binary_ops_t binary_ops` - Stores a list of binary arithmetic operator functions
for the given type, so that operations like filter can accurately be applied
to the type. Check [arithmetic_ops.h](../libconfluo/confluo/types/arithmetic_ops.h)
for examples of binary operator functions that can be defined.
* `unary_ops_t unary_ops` - Stores a list of unary arithmetic operator functions
for the given type, so that operations like filter can work for the given
type. Check [arithmetic_ops.h](../libconfluo/confluo/types/arithmetic_ops.h) for
examples of unary function operators that can be defined.
* `key_op_t key_transform_op` - Stores the key-transform function. This function 
is important for looking up attributes of the type in an index; see 
[key_ops.h](../libconfluo/confluo/types/key_ops.h) for example
definitions of key_transform.
* `parse_op_t parse_op` - Parses data instance from a string representation of this type. See 
[string_ops.h](../libconfluo/confluo/types/string_ops.h) for examples.
* `to_string_op_t to_stirng_op` - Converts data instance of the type to its string representation. See 
[string_ops.h](../libconfluo/confluo/types/string_ops.h) for examples.
* `serialize_op_t serialize_op` - Serializes the underlying data representation of the type into raw bytes;
see [serde_ops.h](../libconfluo/confluo/types/serde_ops.h) for examples.
* `deserialize_op_t deserialize_op` - Reads the raw byte representation of the type and parses it to data;
see [serde_ops.h](../libconfluo/confluo/types/serde_ops.h) for examples.

Example declarations of user-defined types can be found at
[ip_address.h](../libconfluo/test/types/ip_address.h) and 
[size_type.h](../libconfluo/test/size_type.h).

Once the properties for custom type is defined in the `type_properties` struct, 
it needs to be registered with Confluo's [type manager](../libconfluo/confluo/types/type_manager.h) 
via the `type_manager::register_type` interface. Once registered, a useful symbolic
reference to the data type, wrapped in a `data_type` object, can be obtained via
the `type_manager::get_type` interface.

With this object, it is possible to add new columns of this type in any 
Atomic MultiLog. From here on out, appending records to the
Atomic MultiLog, along with operations like filters and triggers, will work out
of the box.

Check [type_manager_test.h][../libconfluo/test/types/type_manager_test.h] for
examples of how to build a schema and add records with user-defined
types.









