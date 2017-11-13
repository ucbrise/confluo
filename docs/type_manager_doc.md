# Type Manager Documentation

The system supports custom user-defined types. This allows you to
create a custom type and have operations like applying filters and triggers
work on that custom type. 

To create a type you need to define the following functions/data so that
native operations can be supported. It is recommended that you create this
in a new namespace so that it doesn't conflict with the same functions
that are defined in the dialog namespace.
* void serialize(std::ostream&, data&) - Compresses the underlying data
representation of the type into bytes to be written to the stream
* void deserialize(std::istream&, data&) - Reads the byte representation
of the type and parses it to data
* rel_ops_t get_relops() - Returns a list of relational operator functions
for the given type, so that operations like filter can work. Check 
[rel_ops.h][../libdialog/dialog/rel_ops.h] for examples of
what relational functions need to be defined.
* binary_ops_t get_binaryops() - Returns a list of binary operator functions
for the given type, so that operations like filter can accurately be applied
to the type. Check [arithmetic_ops.h][../libdialog/dialog/arithmetic_ops.h]
for examples of binary operator functions that need to be defined.
* unary_ops_t get_unaryops() - Returns a list of unary operator functions
for the given type, so that operations like filter can work for the given
type. Check [arithmetic_ops.h][../libdialog/dialog/arithmetic_ops.h] for
examples of unary function operators that need to be defined.
* key_op get_keyops() - Returns a list with a single element containing
the key_transform function. This function is important for looking up
types, check [key_ops.h][../libdialog/dialog/key_ops.h] for example
definitions of key_transform. Make sure that the underlying representation
of the type is what's being converted to a byte string and not the actual
type itself.
* size_t size - This is the size of the underlying representation of the
type, not the size of the type itself.
* void\* min - This is a pointer holding the minimum value that the type
can hold. Check [data_typess.h][../libdialog/dialog/data_types.h] under 
the limits namespace to see examples of how to create this pointer.
* void\* max - This is a pointer holding the maximum value that the type
can hold. Check [data_typess.h][../libdialog/dialog/data_types.h] under 
the limits namespace to see examples of how to create this pointer.
* void\* one - This is a pointer holding the step value that the type
can hold. Check [data_typess.h][../libdialog/dialog/data_types.h] under 
the limits namespace to see examples of how to create this pointer.
* void\* zero - This is a pointer holding the zero value that the type
can hold. Check [data_typess.h][../libdialog/dialog/data_types.h] under 
the limits namespace to see examples of how to create this pointer.
* static std::string name() - Returns the name of the type
* data parse(const std::string& str) - Returns a data instance that is
parsed from a string representation of this type

You can see example declarations of user-defined types with
[ip_address.h][../libdialog/test/ip_address.h] and 
[size_type.h][../libdialog/test/size_type.h].

Once your custom type is created then you can retrieve the id of your
type by calling the uint16_t 
type_manager::get_id_from_type_name(std::string name) function and passing
in the type name as a parameter. With this information you can build
a schema using the schema_builder.add_column(data_type, std::string)
function, and this will now specify the columns of the table with your
user-defined type. You can now define a record function that takes in
as parameters the data_type values of the columns specified and returns a 
pointer to a list of these values. From here you can append records to the
dialog table/store and operations like filters and triggers will work out
of the box. These expressions use the parse function you defined to convert
the string into your data type so make sure the string expressions you 
create match how that function is parsing the string to create a data
instance.
Check [type_manager_test.h][../libdialog/test/type_manager_test.h] for
examples of how to build a schema and add records with user-defined
types.









