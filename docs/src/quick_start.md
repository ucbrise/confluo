# Quick Start

In this Quick Start, we will take a look at how to download and setup Confluo,
load some sample data, and query it.

## Pre-requisites

* MacOS X or Unix-based OS; Windows is not yet supported.
* C++ compiler that supports C++11 standard
* CMake 2.8 or later
* Boost 1.53 or later

For python client, you will additionally require:

* Python 2.7 or later
* Python Packages: six 1.7.2 or later

For java client, you will additionally require:

* Java 1.7 or later
* ant 1.6.2 or later

## Download and Install

To download and install Confluo, use the following commands:

```bash
git clone https://github.com/ucbrise/confluo.git
cd confluo
mkdir build
cd build
cmake ..
make -j && make test && make install
```

## Using Confluo

Confluo can be used in two modes -- embedded and stand-alone. In the embedded mode,
Confluo is used as a header-only library in C++, allowing Confluo to use the same
address-space as the application process. In the stand-alone mode, Confluo runs
as a daemon server process, allowing clients to communicate with it using 
[Apache Thrift](https://thrift.apache.org) protocol.

### Embedded Mode

In order to use Confluo in the embedded mode, we simply need to include
Confluo's header files under libconfluo/confluo, use the Confluo C++ API in 
a C++ application, and compile using a modern C++ compiler. The entry point 
header file to include is `confluo_store.h`. 

We will first create a new Confluo Store with the data path for it to use 
as follows:

```cpp
confluo::confluo_store store("/path/to/data");
```

We then create a new Atomic MultiLog within the Store (synonymous to a database
table); this requires three parameters: name, schema, and the storage mode:

```cpp
std::string schema = "{
  timestamp: LONG,
  op_latency_ms: DOUBLE,
  cpu_util: DOUBLE,
  mem_avail: DOUBLE,
  log_msg: STRING(100)
}";
auto storage_mode = confluo::storage::IN_MEMORY;
store.create_atomic_multilog("perf_log", schema, storage_mode);
```

Our schema contains 5 attributes: a signed 8-byte integer timestamp,
double floating-point precision operation latency (in ms), CPU utilization
and available memory, and a string log message field (upper bounded to 100 
characters). 

We then obtain a reference to our newly created Atomic MultiLog:

```cpp
confluo::atomic_multilog* mlog = store.get_atomic_multilog("perf_log");
```

We can define indexes on the Atomic MultiLog as follows:

```cpp
mlog->add_index("op_latency_ms");
```

to add an index on `op_latency_ms` attribute. We can also install filters as follows:

```cpp
mlog->add_filter("low_resources", "cpu_util>0.8 || mem_avail<0.1");
```

to explicitly filter out records that indicate low system resources (CPU 
utilization > 80%, Available Memory < 10%), using a filter named `low_resources`.

Additionally, we can add aggregates on filters as follows:

```cpp
mlog->add_aggregate("max_latency_ms", "low_resources", "MAX(op_latency_ms)");
```

This adds a new stored aggregate `max_latency_ms` on the filter 
`low_resources` we defined before. In essence, it records the highest 
operation latency reported in any record that also indicated low 
available resources.

Finally, we can install a trigger on aggregates as follows:

```cpp
mlog->install_trigger("high_latency_trigger", "max_latency > 1000");
```

This installs a trigger `high_latency_trigger` on the aggregate 
`max_latency_ms`, which should generate an alert whenever the 
condition `max_latency_ms > 1000` is satisfied, i.e.,
whenever the maximum latency for an operation exceeds 1s and
the available resources are low.

We are now ready to load some data into this multilog:

```cpp
size_t off1 = mlog->append({"100", "0.5", "0.9",  "INFO: Launched 1 tasks"});
size_t off2 = mlog->append({"500", "0.9", "0.05", "WARN: Server {2} down"});
size_t off3 = mlog->append({"1001", "0.9", "0.03", "WARN: Server {2, 4, 5} down"});
```

Note that the `append` method takes a vector of strings as its input, where
the vector corresponds to a single record. The number of entries in the
vector must match the number of entries in the schema, with the exception
of the timestamp --- if the timestamp is not provided, Confluo will automatically
assign one.

Also note that the operation returns a unique offset corresponding to each append
operation. This forms the "key" for records stored in the Atomic MultiLog -- records
can be retrieved by specifying their corresponding offsets.

Now we take a look at how we can query the data in the Atomic MultiLog. First,
it is straightforward to retrieve records given their offsets:

```cpp
auto record1 = mlog->read(off1);
auto record2 = mlog->read(off2);
auto record3 = mlog->read(off3);
```

Each of `record1`, `record2`, and `record3` are vectors of strings.

We can query indexed attributes as follows:

```cpp
auto record_stream1 = mlog->execute_filter("cpu_util>0.5 || mem_avail<0.5");
for (auto s = record_stream1; !s.empty(); s = s.tail()) {
  std::cout << s.head().to_string();
}
```

Note that the operation returns a lazily evaluated stream, which supports
functional style operations like map, filter, etc. See 
[stream.h](https://github.com/ucbrise/confluo/blob/single-machine/libconfluo/confluo/container/lazy/stream.h)
for more details.

We can also query the defined filter as follows:

```cpp
auto record_stream2 = mlog->query_filter("low_resources", 0, UINT64_MAX);
for (auto s = record_stream2; !s.empty(); s = s.tail()) {
  std::cout << s.head().to_string();
}
```

The first parameter corresponds to the name of the filter to be queried, while 
the second and third parameters correspond to the begining timestamp and end 
timestamp to consider for records in the filter. We've specified them to capture
all possible values of timestamp. Similar to the `execute_filter` query, this
operation also returns a lazily evaluated record stream.

We query aggregates as follows:
```cpp
auto value = mlog->get_aggregate("max_latency_ms", 0, UINT64_MAX);
std::cout << value.to_string();
```

The query takes the name of the aggregate as its first parameter, while the 
second and third parameters correspond to begin and end timestmaps, as before.
The query returns a [`numeric`](https://github.com/ucbrise/confluo/blob/single-machine/libconfluo/confluo/types/numeric.h) 
object, which is a wrapper around numeric values.

Finally, we can query the generated alerts by triggers we have installed as
follows:

```cpp
auto alert_stream = mlog->get_alerts(0, UINT64_MAX, "high_latency_trigger");
for (auto s = alert_stream; !s.empty(); s = s.tail()) {
  std::cout << s.head().to_string();
}
```

The query takes and begin and end timestamps as its first and second arguments,
and an optional trigger name as its third argument. The query returns a lazy 
stream over generated alerts for this trigger in the specified time-range.

See API docs for [C++](cpp_api.md) and in-depth user guides on [Data Storage](loading_data.md)
and [Conflo Queries](queries.md) for details on Confluo's supported operations.

### Stand-alone Mode

In the stand-alone mode, Confluo runs as a daemon server, serving client requests
using Apache Thrift protocol. To start the server, run:

```bash
confuod --address=127.0.0.1 --port=9090
```

Once the server daemon is running, you can query it using the C++, Python or Java 
client APIs. The client APIs closely resemble the embedded API. In this guide, 
we will focus on the C++ client API, although client APIs for Python and Java are
almost identical.

We first create a new client connection to the Confluo daemon:

```cpp
confluo::rpc::rpc_client client("127.0.0.1", 9090);
```

The first argument to the `rpc_client` constructor corresponds to the server
hostname, while the second argument corresponds to the server port.

We then create a new Atomic MultiLog within the Store (synonymous to a database
table); as before, this requires three parameters: a name for the Atomic 
MultiLog, a fixed schema, and a storage mode:

```cpp
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

This operation also internally sets the current Atomic MultiLog 
for the client to the one we just created (i.e., `perf_log`).

We can define indexes as follows:

```cpp
client.add_index("op_latency_ms");
```

We can also install filters as follows:

```cpp
client.add_filter("low_resources", "cpu_util>0.8 || mem_avail<0.1");
```

Additionally, we can add aggregates on filters as follows:

```cpp
client.add_aggregate("max_latency_ms", "low_resources", "MAX(op_latency_ms)");
```

Finally, we can install a trigger on an aggregate as follows:

```cpp
client.install_trigger("high_latency_trigger", "max_latency > 1000");
```

To load data into the Atomic MultiLog:

```cpp
size_t off1 = client.append({"100", "0.5", "0.9",  "INFO: Launched 1 tasks"});
size_t off2 = client.append({"500", "0.9", "0.05", "WARN: Server {2} down"});
size_t off3 = client.append({"1001", "0.9", "0.03", "WARN: Server {2, 4, 5} down"});
```

Querying data in the Atomic MultiLog is also similar to the Embedded mode API.

It is straightforward to retrieve records given their offsets:

```cpp
auto record1 = client.read(off1);
auto record2 = client.read(off2);
auto record3 = client.read(off3);
```

We can query indexed attributes as follows:

```cpp
auto record_stream = client.execute_filter("cpu_util>0.5 || mem_avail<0.5");
for (auto s = record_stream; !s.empty(); ++s) {
  std::cout << s.get().to_string();
}
```

We can query a pre-defined filter as follows:

```cpp
auto record_stream = client.query_filter("low_resources", 0, UINT64_MAX);
for (auto s = record_stream; !s.empty(); ++s) {
  std::cout << s.get().to_string();
}
```

We can obtian the value of a pre-defined aggregate as follows:

```cpp
std::string value = client.get_aggregate("max_latency_ms", 0, UINT64_MAX);
std::cout << value;
```

Finally, we can obtain alerts generated by triggers installed on an Atomic 
MultiLog as follows:

```cpp
auto alert_stream = client.get_alerts(0, UINT64_MAX, "high_latency_trigger");
for (auto s = alert_stream; !s.empty(); ++s) {
  std::cout << s.get();
}
```