# Querying Data

Queries in Confluo can either be _online_ or _offline_. Online queries are executed as new data records
are written to an Atomic MultiLog, while offline queries are evaluated on already written records.
In essence, online queries are similar to continuous queries databases.

In order to support online and offline queries, Confluo makes use of indexes, filters, aggregates 
and triggers. The interface for adding these elements to an Atomic MultiLog was described in the 
guide on [Data Storage and Loading](loading_data.md).

To see how Confluo supports online and offline queries, see the individual guides at:

* [Online Queries](online_queries.md)
* [Offline Queries](offline_queries.md)



