# Offline Queries

Offline queries in Confluo are evaluated during runtime, i.e., they are executed
_on-the-fly_ on already written data in response to user requests. The execution
of these queries rely on the raw data and attribute indexes. The guide on 
[Data Storage](loading_data.md) describes how attribute indexes can be
added to Atomic MultiLogs. In this guide, we will focus on how we can query
this data.

## Embedded Mode

We start with Confluo's embedded mode of operation. We work with the assumption that
the Atomic MultiLog has already been created, and filters, aggregates and triggers
have been added to it as outlined in [Data Storage](loading_data.md).

To start with, we have a reference to the Atomic MultiLog as follows:

```cpp
confluo::atomic_multilog* mlog = store.get_atomic_multilog("perf_log");
```

### Retrieving Records

It is straightforward to retrieve records given their offsets:

```cpp
auto record1 = mlog->read(off1);
auto record2 = mlog->read(off2);
auto record3 = mlog->read(off3);
```

Each of `record1`, `record2`, and `record3` are vectors of strings.

### Evaluating Ad-hoc Filter Expressions

We can query indexed attributes as follows:

```cpp
auto record_stream = mlog->execute_filter("cpu_util>0.5 || mem_avail<0.5");
for (auto s = record_stream; !s.empty(); s = s.tail()) {
  std::cout << s.head().to_string();
}
```

The query takes as its argument a filter expression; see the guide on [Data Storage](loading_data.md) 
for details on the elements of the filter expression. The operation returns a lazily evaluated stream, 
which supports functional style operations like map, filter, etc. See 
[Stream API](https://github.com/ucbrise/confluo/blob/single-machine/libconfluo/confluo/container/lazy/stream.h)
for more details.

## Stand-alone Mode

The API for Stand-alone mode of operation is quite similar to the embedded mode.
We only focus on the C++ Client API, since Python and Java Client APIs are
almost identical to the C++ Client API.

As with the embedded mode, we work with the assumption that the client is connected to the server, 
has already created the Atomic MultiLog, and added all relevant filters, aggregates and triggers.
Also, the current Atomic MultiLog for the client has been set to `perf_log` as follows:

```cpp
client.set_current_atomic_multilog("perf_log");
```

### Retrieving Records

It is straightforward to retrieve records given their offsets:

```cpp
auto record1 = client.read(off1);
auto record2 = client.read(off2);
auto record3 = client.read(off3);
```

Each of `record1`, `record2`, and `record3` are vectors of strings.

### Evaluating Ad-hoc Filter Expressions

We can query indexed attributes as follows:

```cpp
auto record_stream = client.execute_filter("cpu_util>0.5 || mem_avail<0.5");
for (auto s = record_stream; !s.empty(); ++s) {
  std::cout << s.get().to_string();
}
```

This operation returns a lazy stream of records, which automatically fetches
more data from the server as the clients consumes them.