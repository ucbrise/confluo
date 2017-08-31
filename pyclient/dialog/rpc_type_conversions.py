from dialog.ttypes import rpc_column
from dialog.schema import schema_builder, schema
from dialog.data_types import data_type

def convert_to_rpc_schema(schema):
    rpc_columns = []
    for column in schema.columns_:
        rpc_col = rpc_column(column.data_type_.type_id_, column.data_type_.size_, column.name_)
        rpc_columns.append(rpc_col)
    return rpc_columns

def convert_to_schema(rpc_schema):
    builder = schema_builder()
    for column in rpc_schema:
        builder.add_column(data_type(column.type_id, column.type_size), column.name)
    return schema("", builder.build())
