#include "archival/archival_metadata.h"

namespace confluo {
namespace archival {

filter_aggregates_archival_metadata::filter_aggregates_archival_metadata(byte_string ts_block,
                                                                         size_t version,
                                                                         size_t num_aggs)
    : ts_block_(ts_block), version_(version), num_aggs_(num_aggs) {
}

filter_aggregates_archival_metadata filter_aggregates_archival_metadata::read(incremental_file_reader &reader) {
  byte_string ts_block = byte_string(reader.read(sizeof(uint64_t)));
  size_t version = reader.read<size_t>();
  size_t num_aggs = reader.read<size_t>();
  return filter_aggregates_archival_metadata(ts_block, version, num_aggs);
}

void filter_aggregates_archival_metadata::append(filter_aggregates_archival_metadata metadata,
                                                 incremental_file_writer &writer) {
  writer.append<uint8_t>(metadata.ts_block_.data(), sizeof(uint64_t));
  writer.append<size_t>(metadata.version_);
  writer.append<size_t>(metadata.num_aggs_);
}

byte_string filter_aggregates_archival_metadata::ts_block() {
  return ts_block_;
}

size_t filter_aggregates_archival_metadata::version() {
  return version_;
}

size_t filter_aggregates_archival_metadata::num_aggregates() {
  return num_aggs_;
}

}
}