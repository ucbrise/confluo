#ifndef CONFLUO_ARCHIVAL_FILTER_ARCHIVER_H_
#define CONFLUO_ARCHIVAL_FILTER_ARCHIVER_H_

#include "aggregate/aggregate_ops.h"
#include "aggregated_reflog.h"
#include "archival_actions.h"
#include "archival_metadata.h"
#include "archiver.h"
#include "conf/configuration_params.h"
#include "compression/confluo_encoder.h"
#include "filter.h"
#include "storage/ptr_aux_block.h"
#include "storage/ptr_metadata.h"
#include "container/reflog.h"
#include "io/incremental_file_reader.h"
#include "io/incremental_file_writer.h"
#include "archival_utils.h"

namespace confluo {
namespace archival {

using namespace compression;
using namespace storage;

class filter_archiver : public archiver {

 public:
  /**
   * Constructor.
   * @param path directory to archive in
   * @param filter filter to archive
   */
  filter_archiver(const std::string &path, monitor::filter *filter);

  /**
   * Attempt to archive filter from the current archival tail up to a data log offset.
   * @param offset data log offset
   */
  void archive(size_t offset);

 private:

  /**
   * Archive unarchived buckets of a reflog up to an offset.
   * @param key radix tree key to which reflog belongs
   * @param refs reflog to archive
   * @param offset data log offset
   * @return data log archival tail
   */
  size_t archive_reflog(byte_string key, reflog &refs, size_t offset);

  /**
   * Archives a reflog bucket of a reflog corresponding to a radix tree key.
   * @param key key to which reflog belongs to in radix_tree
   * @param reflog reflog to which bucket belongs
   * @param bucket reflog bucket starting at idx
   * @param offset max data log offset in bucket
   * @return reflog index to which bucket is archived
   */
  void archive_bucket(byte_string key, reflog &refs, uint64_t *bucket, size_t offset);

  /**
   * Archive aggregates of an aggregated reflog.
   * @param key radix tree key to which reflog belongs
   * @param reflog aggregated reflog
   * @param version version to get aggregates for
   */
  void archive_reflog_aggregates(byte_string key, aggregated_reflog &reflog, size_t version);

 private:
  monitor::filter *filter_;
  incremental_file_writer refs_writer_;
  incremental_file_writer aggs_writer_;

  size_t refs_tail_; // data in the current reflog up to this tail has been archived
  uint64_t ts_tail_; // reflogs in the filter up to this timestamp have been archived

};

class filter_load_utils {

 public:
  /**
   * Load filter's reflogs archived on disk.
   * @param path path to load reflogs from
   * @param filter filter to load into
   * @return data log offset until which radix tree has been loaded
   */
  static size_t load_reflogs(const std::string &path, filter::idx_t &filter);

  /**
   * Load aggregates of reflogs in radix tree archived on disk.
   * @param path path to archived filter
   * @param tree radix tree
   * @return data log offset until which radix tree aggregates have been loaded
   */
  template<typename radix_tree>
  static size_t load_reflog_aggregates(const std::string &path, radix_tree &tree) {
    incremental_file_reader reader(path, "filter_aggs");
    byte_string cur_key;
    while (reader.has_more()) {
      auto action = filter_aggregates_archival_action(reader.read_action<std::string>());
      auto archival_metadata = filter_aggregates_archival_metadata::read(reader);
      cur_key = archival_metadata.ts_block();
      size_t num_aggs = archival_metadata.num_aggregates();

      if (num_aggs > 0) {
        size_t size = sizeof(aggregate) * num_aggs;
        ptr_aux_block aux(state_type::D_ARCHIVED, encoding_type::D_UNENCODED);
        auto *archived_aggs = static_cast<aggregate *>(allocator::instance().alloc(size, aux));
        storage::lifecycle_util<aggregate>::construct(archived_aggs);
        for (size_t i = 0; i < num_aggs; i++) {
          data_type type = reader.read<data_type>();
          std::string data = reader.read(type.size);
          archived_aggs[i] = aggregate(type, aggregators::sum_aggregator(), 1);
          archived_aggs[i].seq_update(0, numeric(type, &data[0]), archival_metadata.version());
        }
        tree.get_unsafe(cur_key)->init_aggregates(num_aggs, archived_aggs);
      }
    }
    reader.truncate(reader.tell(), reader.tell_transaction_log());
    auto last = tree.get(cur_key);
    return last ? *std::max_element(last->begin(), last->end()) : 0;
  }

 private:
  /**
   * Initialize a bucket.
   * @param refs reflog
   * @param idx reflog index
   * @param encoded_bucket bucket to initialize at index
   */
  static void init_bucket_ptr(reflog *refs, size_t idx, encoded_reflog_ptr encoded_bucket);

};

}
}

#endif /* CONFLUO_ARCHIVAL_FILTER_ARCHIVER_H_ */
