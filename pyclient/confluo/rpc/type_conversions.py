from data_types import DataType
from schema import SchemaBuilder, Schema
from ttypes import rpc_column


def convert_to_rpc_schema(schema):
    """ Converts a schema to an rpc schema for the client.

    Args:
        schema: The schema to convert.
    Returns:
        The rpc schema.
    """
    rpc_columns = []
    for column in schema.columns_:
        rpc_col = rpc_column(column.data_type_.type_id_, column.data_type_.size_, column.name_)
        rpc_columns.append(rpc_col)
    return rpc_columns


def convert_to_schema(rpc_schema):
    """ Converts an rpc schema to a schema for confluo.

    Args:
        rpc_schema: The rpc_schema to convert.
    Returns:
        The schema for confluo.
    """
    builder = SchemaBuilder()
    for column in rpc_schema:
        builder.add_column(DataType(column.type_id, column.type_size), column.name)
    return Schema(builder.build())
