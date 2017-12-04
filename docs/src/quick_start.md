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

In order to use Confluo in the embedded mode, you simply need to include
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
std::string schema("{ type: CHAR, msg: STRING(100) }");
auto storage_mode = confluo::storage::IN_MEMORY;
store.create_atomic_multilog("my_multilog", schema, storage_mode);
```

Our simple schema only contains two attributes: a character type attribute, and 
a string message field. Confluo will also automatically create a timestamp 
field if the schema does not explicityly specify one. 

We then obtain a reference to our newly created Atomic MultiLog:

```cpp
confluo::atomic_multilog* mlog = store.get_atomic_multilog("my_multilog");
```

We can define indexes on the Atomic MultiLog as follows:

```cpp
mlog->add_index("type")
```

to add an index on "type" attribute. We can also install filters as follows:

```cpp
mlog->add_filter("type_a", "type == a");
```

to explicitly filter out records that have the "type" attribute value "a", using
a filter named "type\_a". 

Additionally, we can add aggregates on filters as follows:

```cpp
mlog->add_aggregate("latest_a", "type_a", "MAX(timestamp)")
```
This adds a new stored aggregate "latest\_a" on the filter "type\_a" we defined 
before. In essence, it records the latest timestamp of any record that has the
type attribute value "a".

Finally, we can add a trigger on aggregates as follows:

```cpp
mlog->add_trigger("trigger_a", "lastest_a > 0");
```

This adds a trigger "trigger\_a" on the aggregate "latest\_a", which should 
generate an alert whenever the condition latest\_a > 0 is satisfied, i.e.,
whenever any record with  type = a is written to the Atomic MultiLog.

We are now ready to load some data into this multilog:

```cpp
size_t off1 = mlog->append({"b", "Hello World!");
size_t off2 = mlog->append({"b", "How are you doing today?");
size_t off3 = mlog->append({"a", "Good to see you!"});
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
auto record_stream1 = mlog->execute_filter("type == b");
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
auto record_stream2 = mlog->query_filter("type_a", 0, UINT64_MAX);
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
auto value = mlog->get_aggregate("latest_a", 0, UINT64_MAX);
std::cout << value.to_string();
```

The query takes the name of the aggregate as its first parameter, while the 
second and third parameters correspond to begin and end timestmaps, as before.
The query returns a [`numeric`](../libconfluo/confluo/types/numeric.h) object,
which is a wrapper around numeric values.

Finally, we can query the generated alerts by triggers we have installed as 
follows:
```cpp
auto alert_stream = mlog->get_alerts(0, UINT64_MAX, "trigger_a");
for (auto s = alert_stream; !s.empty(); s = s.tail()) {
  std::cout << s.head().to_string();
}
```

The query takes and begin and end timestamps as its first and second arguments,
and an optional trigger name as its third argument. The query returns a lazy 
stream over generated alerts for this trigger in the specified time-range.

See [`confluo_store.h`](https://github.com/ucbrise/confluo/blob/single-machine/libconfluo/confluo/conflu_store.h)
and [`atomic_multilog.h`](https://github.com/ucbrise/confluo/blob/single-machine/libconfluo/confluo/atomic_multilog.h)
to see all the APIs described here, along with variations and other caveats.

### Stand-alone Mode

In the stand-alone mode, Confluo runs as a daemon server, serving client requests
using Apache Thrift protocol. To start the server, run:

```bash
confuod
```

you can find options supported by the binary using:

```bash
confluod --help
```

Once the server daemon is running, you can query it using the C++ or Python 
client APIs. The client APIs closely resemble the embedded API, and can be
found [here](https://github.com/ucbrise/confluo/blob/single-machine/librpc/rpc/rpc_client.h) 
for the C++ Client, and [here](https://github.com/ucbrise/confluo/blob/single-machine/pyclient/confluo/rpc/rpc_client.py) 
for the Python Client.
