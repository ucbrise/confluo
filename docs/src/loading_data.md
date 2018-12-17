# Data Storage

Confluo operates on _data streams_. Each stream comprises of records, each of
which follows a pre-defined schema over a collection of strongly-typed 
attributes.

## Attributes

Confluo currently supports only _bounded-width_ attributes[^1]. An attribute 
is said to have bounded-width if all records in a single stream use some 
maximum number of bits to represent that attribute; this includes primitive 
data types such as binary, integral or floating-point values, or 
domain-specific types such as IP addresses, ports, sensor readings, etc. 
Confluo also requires each record in the stream to have a 8-byte nano-second 
precision timestamp attribute; if the application does not assign timestamps, 
Confluo internally assigns one during the write operation. 

[^1]: We plan on adding support for variable width data types in a future relase.

## Schema

A schema in Confluo is a collection of strongly-typed attributes. It is specified
via JSON like semantics; for instance, consider the example below for a simple schema 
with three attributes: 

```json
{
  timestamp: ULONG,
  op_latency_ms: DOUBLE,
  cpu_util: DOUBLE,
  mem_avail: DOUBLE,
  log_msg: STRING(100)
}
```

The first attribute is timestamp with a 8-byte signed integer type; the second, third and
fourth attributes correspond to operation latency (in ms), CPU utilization and available 
memory respectively, all with double precision floating-point type. The final attribute is a
log message, with a string type upper bound by 100 characters. Note that each of the 
attributes must have types associated with them, and each record in a
stream with this schema must have its attributes in this order. While Confluo natively 
supports common primitive types, you can add custom bounded-width data types to Confluo's
type system. More details can be found at the [Confluo Type-System](type_system.md) guide.

## Atomic MultiLog

Atomic MultiLogs are the basic storage abstraction in Confluo, and are similar in
interface to database tables. In order to store data from different streams, 
applications can create an Atomic MultiLog with a pre-specified schema, 
and write data streams that conform to the schema to the Atomic MultiLog.

To support queries, applications can add an index for individual attributes in the schema. 
Confluo also employs a match-action language with three main elements: _filter_, 
_aggregate_  and _trigger_. 

* A Confluo filter is an expression comprising of relational and boolean operators 
(see Table below) over arbitrary subset of bounded-width attributes, and identifies records 
that match the expression. 
* A Confluo aggregate evaluates a computable function on an attribute for all records that 
match a certain filter expression. 
* Finally, a Confluo trigger is a boolean conditional (e.g., <, >, =, etc.) evaluated over 
a Confluo aggregate. 

**Relational Operators in Filters**:

| Operator                 | Examples                       |
| ------------------------ | ------------------------------ | 
| Equality                 | `dst_port=80`                  | 
| Range                    | `cpu_util>0.8`                 | 

**Boolean Operators in Filters**:

| Operator                 | Examples                        |
| ------------------------ | ------------------------------- | 
| Conjunction              | `volt>200 && temp>100`          | 
| Disjunction              | `cpu_util>0.8 || mem_avail<0.1` | 
| Negation                 | `transport_protocol != TCP`     | 

Confluo supports indexes, filters, aggregates and triggers only on bounded-width
attributes in the schema. Once added, each of these are evaluated and updated 
upon arrival of each new batch of data records.

## A Performance Monitoring and Diagnosis Example

We will now see how we can create Atomic MultiLogs, add indexes, filters, aggregate and triggers on them,
and finally load some data into them, for both embedded and stand-alone modes of operation. We will work
with the example of a performance monitoring and diagnosis tool using Confluo.

### Embedded mode

In order to use Confluo in the embedded mode, we simply need to include
Confluo's header files under libconfluo/confluo, use the Confluo C++ API in 
a C++ application, and compile using a modern C++ compiler. The entry point 
header file to include is `confluo_store.h`. 

#### Creating a New Confluo Store

We will first create a new Confluo Store with the data path for it to use 
as follows:

```cpp
confluo::confluo_store store("/path/to/data");
```

#### Creating a New Atomic MultiLog

We then create a new Atomic MultiLog within the Store (synonymous to a database
table); this requires three parameters: a name for the Atomic MultiLog, a fixed
schema, and a storage mode:

```cpp
std::string schema = "{
  timestamp: ULONG,
  op_latency_ms: DOUBLE,
  cpu_util: DOUBLE,
  mem_avail: DOUBLE,
  log_msg: STRING(100)
}";
auto storage_mode = confluo::storage::IN_MEMORY;
store.create_atomic_multilog("perf_log", schema, storage_mode);
```

Our Atomic MultiLog adopts the same schema outlined [above](loading_data.md#schema). The 
storage mode is set to in-memory, but can be of the following types:

|   Storage Mode  |                                                                                                                                 Description                                                                                                                                |
|:---------------:|:---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
|    IN_MEMORY    |                                                                                    All data is written purely in memory, and no attempt is made at persisting data to secondary storage.                                                                                   |
|     DURABLE     |                            Only the raw data (i.e., raw bytes corresponding to each record) is persisted to secondary storage for each write. The write is not considered complete unless its effects have been persisted to secondary storage.                            |
| DURABLE_RELAXED | Only the raw data (i.e., raw bytes corresponding to each record) is persisted to secondary storage; however, the data is buffered in memory and only persisted periodically, instead of persisting data for every write. This generally leads to better write performance. |

We then obtain a reference to our newly created Atomic MultiLog:

```cpp
confluo::atomic_multilog* mlog = store.get_atomic_multilog("perf_log");
```

#### Adding Indexes

We can define indexes on the Atomic MultiLog as follows:

```cpp
mlog->add_index("op_latency_ms");
```

to add an index on `op_latency_ms` attribute. 

#### Adding Filters

We can also install filters as follows:

```cpp
mlog->add_filter("low_resources", "cpu_util>0.8 || mem_avail<0.1");
```

to explicitly filter out records that indicate low system resources (CPU 
utilization > 80%, Available Memory < 10%), using a filter named `low_resources`. 

#### Adding Aggregates

Additionally, we can add aggregates on filters as follows:

```cpp
mlog->add_aggregate("max_latency_ms", "low_resources", "MAX(op_latency_ms)");
```

This adds a new stored aggregate `max_latency_ms` on the filter 
`low_resources` we defined before. In essence, it records the highest 
operation latency reported in any record that also indicated low 
available resources.

#### Installing Triggers

Finally, we can install a trigger on aggregates as follows:

```cpp
mlog->install_trigger("high_latency_trigger", "max_latency > 1000");
```

This installs a trigger `high_latency_trigger` on the aggregate 
`max_latency_ms`, which should generate an alert whenever the 
condition `max_latency_ms > 1000` is satisfied, i.e.,
whenever the maximum latency for an operation exceeds 1s and
the available resources are low.

#### Loading sample data into Atomic MultiLog

We are now ready to load some data into this Atomic MultiLog. Atomic MutliLogs
only support addition of new data via _appends_. However, new data can be
appended in several ways:

##### Appending String Vectors

This version of `append` method takes a vector of strings as its input, where
the vector corresponds to a single record. The number of entries in the
vector must match the number of entries in the schema, with the exception
of the timestamp --- if the timestamp is not provided, Confluo will automatically
assign one.

```cpp
size_t off1 = mlog->append({"100", "0.5", "0.9",  "INFO: Launched 1 tasks"});
size_t off2 = mlog->append({"500", "0.9", "0.05", "WARN: Server {2} down"});
size_t off3 = mlog->append({"1001", "0.9", "0.03", "WARN: Server {2, 4, 5} down"});
```

Also note that the operation returns a unique offset corresponding to each append
operation. This forms the "key" for records stored in the Atomic MultiLog -- records
can be retrieved by specifying their corresponding offsets.

##### Appending Raw bytes

This version of `append` takes as its input a pointer to a C/C++ struct, that maps
exactly to the Atomic MultiLog's schema. For instance, our schema would map to the following
C/C++ struct:

```cpp
struct perf_log_record {
  int64_t timestamp;
  double op_latency_ms;
  double cpu_util;
  double mem_avail;
  char log_msg[100];
};
```

Note that `log_msg` maps to a `char[100]` rather than an `std::string`. To add a new record, we
would populate a struct instance, and pass its reference to the append function:

```cpp
int64_t ts = utils::time_utils::cur_ns();
perf_log_record rec = { ts, 2000.0, 0.95, 0.01, "WARN: Server {2, 4, 5} down" };
size_t off4 = mlog->append(&rec);
```

Note that this is a more efficient variant of append, since it avoids the overheads of parsing 
strings to the corresponding attribute data types.

##### Batched Appends

It is also possible to batch multiple record appends into a single append. The first
step in building a batch is to obtain a batch builder:

```cpp
auto batch_bldr = mlog->get_batch_builder();
```

The batch builder supports adding new records via both string vector and raw byte interfaces:

```cpp
batch_bldr.add_record({ "400", "0.85", "0.07", "WARN: Server {2, 4} down"});
perf_log_record rec = { utils::time_utils::cur_ns(), 100.0, 0.65, 0.25, "WARN: Server {2} down" };
batch_bldr.add_record(&rec);
```

Once the batch is populated, we can append the batch to the Atomic MultiLog as follows:

```cpp
size_t off5 = mlog->append_batch(batch_bldr.get_batch());
```

To understand how we can query the data we have loaded so far, read the guide on [Confluo Queries](queries.md).

### Stand-alone Mode

In the stand-alone mode, Confluo runs as a daemon server, serving client requests
using Apache Thrift protocol. To start the server, run:

```bash
confuod --address=127.0.0.1 --port=9090
```

Once the server daemon is running, you can send requests to it using the 
C++/Python/Java client APIs. Note that the C++ Client API is almost identical to 
the embedded mode API.

We look at the same performance monitoring and diagnosis tool example for the
stand-alone mode. The relevant header file to include for the C++ Client API is
`rpc_client.h`.

#### Creating a Client Connection

To begin with, we first have to establish a client connection with the server.

```cpp tab="C++"
confluo::rpc::rpc_client client("127.0.0.1", 9090);
```

```python tab="Python"
from confluo.rpc.client import RpcClient

client = RpcClient("127.0.0.1", 9090)
```

The first argument to the `rpc_client` constructor corresponds to the server
hostname, while the second argument corresponds to the server port.


#### Creating a New Atomic MultiLog

We then create a new Atomic MultiLog within the Store (synonymous to a database
table); as before, this requires three parameters: a name for the Atomic 
MultiLog, a fixed schema, and a storage mode:

```cpp tab="C++"
std::string schema = "{
  timestamp: LONG,
  op_latency_ms: DOUBLE,
  cpu_util: DOUBLE,
  mem_avail: DOUBLE,
  log_msg: STRING(100)
}";
auto storage_mode = confluo::storage::IN_MEMORY;
client.create_atomic_multilog("perf_log", schema, storage_mode);
```

```python tab="Python"
from confluo.rpc.storage import StorageMode

schema = """{
  timestamp: ULONG,
  op_latency_ms: DOUBLE,
  cpu_util: DOUBLE,
  mem_avail: DOUBLE,
  log_msg: STRING(100)
}"""
storage_mode = StorageMode.IN_MEMORY
client.create_atomic_multilog("perf_log", schema, storage_mode)
```

This operation also internally sets the current Atomic MultiLog 
for the client to the one we just created (i.e., `perf_log`). It
is also possible to explicitly set the current Atomic MultiLog for
the client as follows:

```cpp tab="C++"
client.set_current_atomic_multilog("perf_log");
```

```python tab="Python"
client.set_current_atomic_multilog("perf_log")
```

!!! note
    It is necessary to set the current Atomic MultiLog for the `rpc_client`.
    Issuing requests via the client without setting the current Atomic MultiLog
    will result in exceptions.

#### Adding Indexes

We can define indexes as follows:

```cpp tab="C++"
client.add_index("op_latency_ms");
```

```python tab="Python"
client.add_index("op_latency_ms")
```

#### Adding Filters

We can also install filters as follows:

```cpp tab="C++"
client.add_filter("low_resources", "cpu_util>0.8 || mem_avail<0.1");
```

```python tab="Python"
client.add_filter("low_resources", "cpu_util>0.8 || mem_avail<0.1")
```

#### Adding Aggregates

Additionally, we can add aggregates on filters as follows:

```cpp tab="C++"
client.add_aggregate("max_latency_ms", "low_resources", "MAX(op_latency_ms)");
```

```python tab="Python"
client.add_aggregate("max_latency_ms", "low_resources", "MAX(op_latency_ms)")
```

#### Installing Triggers

Finally, we can install a trigger on an aggregate as follows:

```cpp tab="C++"
client.install_trigger("high_latency_trigger", "max_latency > 1000");
```

```python tab="Python"
client.install_trigger("high_latency_trigger", "max_latency_ms > 1000")
```

#### Loading sample data into Atomic MultiLog

We are now ready to load some data into the Atomic MultiLog on the server. 

##### Appending String Vectors

```cpp tab="C++"
size_t off1 = client.append({"100", "0.5", "0.9",  "INFO: Launched 1 tasks"});
size_t off2 = client.append({"500", "0.9", "0.05", "WARN: Server {2} down"});
size_t off3 = client.append({"1001", "0.9", "0.03", "WARN: Server {2, 4, 5} down"});
```

```python tab="Python"
off1 = client.append([100.0, 0.5, 0.9,  "INFO: Launched 1 tasks"])
off2 = client.append([500.0, 0.9, 0.05, "WARN: Server {2} down"])
off3 = client.append([1001.0, 0.9, 0.03, "WARN: Server {2, 4, 5} down"])
```

##### Batched Appends

It is also possible to batch multiple record appends into a single append operation via the client API. 
This is particularly useful since batching helps amortize the cost of network latency.

The first step in building a batch is to obtain a batch builder:

```cpp tab="C++"
auto batch_bldr = client.get_batch_builder();
```

```python tab="Python"
batch_bldr = client.get_batch_builder()
```

The batch builder supports adding new records via both string vector and raw byte interfaces:

```cpp tab="C++"
batch_bldr.add_record({ "400", "0.85", "0.07", "WARN: Server {2, 4} down"});
batch_bldr.add_record({ "100", "0.65", "0.25", "WARN: Server {2} down" });
```

```python tab="Python"
batch_bldr.add_record([ 400.0, 0.85, 0.07, "WARN: Server {2, 4} down" ])
batch_bldr.add_record([ 100.0, 0.65, 0.25, "WARN: Server {2} down" ])
```

Once the batch is populated, we can append the batch as follows:

```cpp tab="C++"
size_t off4 = client.append_batch(batch_bldr.get_batch());
```

```python tab="Python"
off4 = client.append_batch(batch_bldr.get_batch())
```

Details on querying the data via the client interface can be found in the guide on [Confluo Queries](queries.md).