namespace java edu.berkeley.cs.streaming
namespace cpp streaming
namespace py streaming

typedef i64 offset_t;
typedef i64 uuid_t;
typedef i64 length_t;

service stream_service {
  void add_stream(1: uuid_t uuid),
  offset_t write(1: uuid_t uuid, 2: binary data),
  binary read(1: uuid_t uuid, 2: offset_t offset, 3: length_t length),
}
