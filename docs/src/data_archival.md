# Archiving Data

Confluo can limit the amount of data resident in memory by archiving old data. It does this by periodically archiving records in the data log, as well as all corresponding filter and index data, up to a data log offset. Archived data is accessed in the same way as data in memory since it is memory-mapped after it is persisted. 

This data is written to disk either in its existing format, or in a compressed representation, depending on configuration parameters. By default, the data log is compressed using LZ4. Filter and index data are delta compressed. During reads, all decompression is done under the hood. 

The archiver does not affect any readers that are concurrently accessing data since pointers to Confluo's data structures have associated reference counts that prevent premature deallocation. Readers obtain these pointers atomically.

## Usage


### Periodic Archival

To initialize a multilog with periodic archiving capabilities:

```cpp
auto archival_mode = confluo::archival::periodic_archival_mode::ON;
store.create_atomic_multilog("my_log", schema, storage::IN_MEMORY, archival_mode);
```

Alternatively, for an existing multilog, we can toggle periodic archival:

```cpp
mlog->set_periodic_archival(confluo::archival::periodic_archival_mode::ON);
mlog->set_periodic_archival(confluo::archival::periodic_archival_mode::OFF);
```

By default, if archival is on, the archiver will run periodically every 5 minutes. This can be changed through configuration parameters. The maximum amount of data log data in memory can also be configured.

```
archival_periodicity_ms 
archival_in_memory_datalog_window_bytes *TODO better name for above param*
```

### Forced Archival

Regardless of whether archival is turned on for a particular multilog, the user can force archival up to a data log offset by calling:

```cpp
mlog->archive(); // Archive up to the read tail
mlog->archive(offset); // Archive up to any offset before the read tail
```

### Allocator-triggered Archival

In cases where the periodic archiver cannot keep up with write pressure and the maximum memory of the system is reached, the allocator will block until Confluo makes more memory available, which is done by archiving the multilogs aggressively. All multilogs are archived in their entirety to make space for newer data. This can be configured by changing:

```
max_memory
``` 
