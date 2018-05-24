#include "archival/index_archiver.h"

namespace confluo {
namespace archival {

index_archiver::index_archiver(const std::string &path, index::radix_index *index, const column_t column)
    : index_(index),
      reflog_tails_(),
      column_(column),
      writer_(path, "index_data", archival_configuration_params::MAX_FILE_SIZE) {
  writer_.close();
}

void index_archiver::archive(size_t offset) {
  writer_.open();
  byte_string min = column_.min().to_key(column_.index_bucket_size());
  byte_string max = column_.max().to_key(column_.index_bucket_size());
  auto reflogs = index_->range_lookup_reflogs(min, max);
  for (auto it = reflogs.begin(); it != reflogs.end(); it++) {
    auto& refs = *it;
    archive_reflog(it.key(), refs, offset);
  }
  writer_.close();
}

void index_archiver::archive_reflog(byte_string key, reflog &refs, size_t offset) {
  size_t reflog_idx = reflog_tails_[key.to_string()];
  size_t data_log_off = 0;
  while (data_log_off < offset && reflog_idx < refs.size()) {
    read_only_reflog_ptr bucket_ptr;
    refs.ptr(reflog_idx - reflog_idx % reflog_constants::BUCKET_SIZE, bucket_ptr);
    // Optimized get: we can assume any pointer in memory is unencoded.
    uint64_t* data = bucket_ptr.get().ptr_as<uint64_t>();
    auto aux = ptr_aux_block::get(ptr_metadata::get(data));
    // Skip if already archived.
    if (aux.state_ != state_type::D_IN_MEMORY) {
      reflog_idx += reflog_constants::BUCKET_SIZE;
      continue;
    }
    if ((data_log_off = archival_utils::max_in_reflog_bucket(data)) < offset) {
      reflog_idx = archive_bucket(key, refs, reflog_idx, data, data_log_off);
    }
  }
  reflog_tails_[key.to_string()] = reflog_idx;
}

size_t index_archiver::archive_bucket(byte_string key, reflog &refs, size_t idx, uint64_t *bucket, size_t offset) {
  auto metadata_copy = *(ptr_metadata::get(bucket));
  size_t bucket_size = std::min(reflog_constants::BUCKET_SIZE, refs.size() - idx);
  auto raw_encoded_bucket = confluo_encoder::encode(bucket, bucket_size * sizeof(uint64_t),
                                                    archival_configuration_params::REFLOG_ENCODING_TYPE);
  size_t enc_size = raw_encoded_bucket.size();
  metadata_copy.data_size_ = static_cast<uint32_t>(enc_size);

  auto archival_metadata = radix_tree_archival_metadata(key, idx, bucket_size);
  auto action = index_archival_action(key, idx + bucket_size, offset);

  radix_tree_archival_metadata::append(archival_metadata, writer_);
  auto off = writer_.append<ptr_metadata, uint8_t>(&metadata_copy, 1, raw_encoded_bucket.get(), enc_size);
  writer_.commit<std::string>(action.to_string());

  // Only swap pointer for full buckets.
  if (bucket_size == reflog_constants::BUCKET_SIZE) {
    ptr_aux_block aux(state_type::D_ARCHIVED, archival_configuration_params::REFLOG_ENCODING_TYPE);
    void* enc_bucket = ALLOCATOR.mmap(off.path(), static_cast<off_t>(off.offset()), enc_size, aux);
    archival_utils::swap_bucket_ptr(refs, idx, encoded_reflog_ptr(enc_bucket));
  }
  return idx + bucket_size;
}

size_t index_load_utils::load(const std::string &path, index::radix_index *index) {
  incremental_file_reader reader(path, "index_data");
  size_t data_log_archival_tail = 0;
  while (reader.has_more()) {
    auto action = index_archival_action(reader.read_action<std::string>());
    data_log_archival_tail = std::max(data_log_archival_tail, action.data_log_archival_tail());
    size_t key_size = action.key_size();

    auto archival_metadata = radix_tree_archival_metadata::read(reader, key_size);
    byte_string cur_key = archival_metadata.key();
    size_t reflog_idx = archival_metadata.reflog_index();

    incremental_file_offset off = reader.tell();
    ptr_metadata metadata = reader.read<ptr_metadata>();
    size_t bucket_size = metadata.data_size_;

    auto*& refs = index->get_or_create(cur_key);
    ptr_aux_block aux(state_type::D_ARCHIVED, archival_configuration_params::REFLOG_ENCODING_TYPE);
    void* encoded_bucket = ALLOCATOR.mmap(off.path(), static_cast<off_t>(off.offset()), bucket_size, aux);
    init_bucket_ptr(refs, reflog_idx, encoded_reflog_ptr(encoded_bucket));
    reader.advance<uint8_t>(bucket_size);
    reflog_idx += archival_metadata.bucket_size();

    atomic::type<size_t>* tail = refs->write_tail();
    size_t old_tail = atomic::load(tail);
    atomic::strong::cas(tail, &old_tail, reflog_idx);

  }
  reader.truncate(reader.tell(), reader.tell_transaction_log());
  return data_log_archival_tail;
}

void index_load_utils::init_bucket_ptr(reflog *refs, size_t idx, encoded_reflog_ptr encoded_bucket) {
  auto& bucket_containers = refs->data();
  size_t bucket_idx, container_idx;
  refs->raw_data_location(idx, container_idx, bucket_idx);
  refs->ensure_alloc(idx, idx);
  auto* container = atomic::load(&bucket_containers[container_idx]);
  encoded_reflog_ptr old_data = container[bucket_idx].atomic_load();
  container[bucket_idx].atomic_init(encoded_bucket, old_data);
}

}
}