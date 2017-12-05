# Modes of Operation

Confluo can be used in two modes -- embedded and stand-alone. 

## Embedded Mode

In the _embedded mode_, Confluo is used as a header-only library in C++, allowing Confluo
to use the same address-space as the application process. This enables ultra low-latency 
writes and queries, but only supports applications written in C++.

## Stand-alone Mode

Confluo also supports a _stand-alone mode_, where Confluo runs as a daemon 
server process and allows clients to communicate with it using 
[Apache Thrift](https://thrift.apache.org) protocol. Operations now incur higher latencies
(due to serialization/deserialization overheads), but can now operate over the network, 
and allows Confluo to store data from applications written in different languages.

## More on Usage

Read more on how you can perform different operations with the two modes of operation:

* [Data Storage and Loading Data](loading_data.md)
* [Querying Data](queries.md)
    - [Online Queries](online_queries.md)
    - [Offline Queries](offline_queries.md)