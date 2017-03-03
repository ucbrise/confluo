#ifndef NETPLAY_PACKETSTORE_H_
#define NETPLAY_PACKETSTORE_H_

#include "logstore.h"

namespace netplay {

/**
 * A data store for packet header data.
 *
 * Stores entire packet headers, along with 'casts' and 'characters' to enable
 * efficient rich semantics. See https://cs.berkeley.edu/~anuragk/netplay.pdf
 * for details.
 */
class packet_store {
 public:

  /** Type definitions **/
  // typedef slog::log_store::handle handle;
  class handle : public slog::log_store::handle {
   public:
    handle(packet_store& store)
        : slog::log_store::handle(store.store_),
          store_(store) {
    }

    void add_src_ip(slog::token_list& list, uint32_t src_ip) {
      list.push_back(slog::token_t(store_.srcip_idx_id_, src_ip));
    }

    void add_dst_ip(slog::token_list& list, uint32_t dst_ip) {
      list.push_back(slog::token_t(store_.dstip_idx_id_, dst_ip));
    }

    void add_src_port(slog::token_list& list, uint16_t src_port) {
      list.push_back(slog::token_t(store_.srcport_idx_id_, src_port));
    }

    void add_dst_port(slog::token_list& list, uint16_t dst_port) {
      list.push_back(slog::token_t(store_.dstport_idx_id_, dst_port));
    }

    void add_timestamp(slog::token_list& list, uint32_t timestamp) {
      list.push_back(slog::token_t(store_.timestamp_idx_id_, timestamp));
    }

   private:
    packet_store& store_;
  };

  /**
   * Constructor to initialize the packet store.
   *
   * By default, the packet store creates indexes on 5 fields:
   * Source IP, Destination IP, Source Port, Destination Port and Timestamp.
   */
  packet_store() {
    srcip_idx_id_ = store_.add_index(4);
    dstip_idx_id_ = store_.add_index(4);
    srcport_idx_id_ = store_.add_index(2);
    dstport_idx_id_ = store_.add_index(2);
    timestamp_idx_id_ = store_.add_index(4);
  }

  /**
   * Get a handle to the packet store. Each thread **must** have its own handle
   * -- handles cannot be shared between threads.
   *
   * @return A handle to the packet store.
   */
  handle* get_handle() {
    return new handle(*this);
  }

 private:
  slog::log_store store_;
  uint32_t srcip_idx_id_;
  uint32_t dstip_idx_id_;
  uint32_t srcport_idx_id_;
  uint32_t dstport_idx_id_;
  uint32_t timestamp_idx_id_;
};

}

#endif /* NETPLAY_PACKETSTORE_H_ */
