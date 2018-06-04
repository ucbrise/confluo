#include "aggregate/aggregate.h"

namespace confluo {

aggregate_node::aggregate_node()
    : aggregate_node(primitive_types::NONE_TYPE(), 0, nullptr) {
}

aggregate_node::aggregate_node(numeric agg, uint64_t version, aggregate_node *next)
    : value_(std::move(agg)),
      version_(version),
      next_(next) {
}

numeric aggregate_node::value() const {
  return value_;
}

uint64_t aggregate_node::version() const {
  return version_;
}

aggregate_node *aggregate_node::next() {
  return next_;
}

aggregate_list::aggregate_list()
    : head_(nullptr),
      agg_(aggregators::invalid_aggregator()),
      type_(primitive_types::NONE_TYPE()) {
}

aggregate_list::aggregate_list(data_type type, aggregator agg)
    : head_(nullptr),
      agg_(std::move(agg)),
      type_(type) {
}

aggregate_list::aggregate_list(const aggregate_list &other)
    : head_(nullptr),
      agg_(other.agg_),
      type_(other.type_) {
  aggregate_node *other_tail = atomic::load(&other.head_);
  while (other_tail != nullptr) {
    aggregate_node* cur_head = atomic::load(&head_);
    void* raw = allocator::instance().alloc(sizeof(aggregate_node));
    aggregate_node* new_head = new(raw) aggregate_node(other_tail->value(), other_tail->version(), cur_head);
    atomic::store(&head_, new_head);
    other_tail = other_tail->next();
  }
}

aggregate_list &aggregate_list::operator=(const aggregate_list &other) {
  head_ = nullptr;
  agg_ = other.agg_;
  type_ = other.type_;
  aggregate_node *other_tail = atomic::load(&other.head_);
  while (other_tail != nullptr) {
    aggregate_node* cur_head = atomic::load(&head_);
    void* raw = allocator::instance().alloc(sizeof(aggregate_node));
    aggregate_node* new_head = new(raw) aggregate_node(other_tail->value(), other_tail->version(), cur_head);
    atomic::store(&head_, new_head);
    other_tail = other_tail->next();
  }
  return *this;
}

aggregate_list::~aggregate_list() {
  aggregate_node *cur_node = atomic::load(&head_);
  while (cur_node != nullptr) {
    aggregate_node* next = cur_node->next();
    cur_node->~aggregate_node();
    allocator::instance().dealloc(cur_node);
    cur_node = next;
  }
}

numeric aggregate_list::get(uint64_t version) const {
  aggregate_node *cur_head = atomic::load(&head_);
  aggregate_node *req = get_node(cur_head, version);
  if (req != nullptr)
    return req->value();
  return agg_.zero;
}

void aggregate_list::comb_update(const numeric &value, uint64_t version) {
  aggregate_node *cur_head = atomic::load(&head_);
  aggregate_node *req = get_node(cur_head, version);
  numeric old_agg = (req == nullptr) ? agg_.zero : req->value();
  void* raw = allocator::instance().alloc(sizeof(aggregate_node));
  aggregate_node* node = new(raw) aggregate_node(agg_.comb_op(old_agg, value), version, cur_head);
  atomic::store(&head_, node);
}

void aggregate_list::seq_update(const numeric &value, uint64_t version) {
  aggregate_node *cur_head = atomic::load(&head_);
  aggregate_node *req = get_node(cur_head, version);
  numeric old_agg = (req == nullptr) ? agg_.zero : req->value();
  void* raw = allocator::instance().alloc(sizeof(aggregate_node));
  aggregate_node* node = new(raw) aggregate_node(agg_.seq_op(old_agg, value), version, cur_head);
  atomic::store(&head_, node);
}

aggregate_node *aggregate_list::get_node(aggregate_node *head, uint64_t version) const {
  if (head == nullptr)
    return nullptr;

  aggregate_node *node = head;
  aggregate_node *ret = nullptr;
  uint64_t max_version = 0;
  while (node != nullptr) {
    if (node->version() == version)
      return node;

    if (node->version() < version && node->version() > max_version) {
      ret = node;
      max_version = node->version();
    }

    node = node->next();
  }
  return ret;
}

aggregate::aggregate()
    : type_(primitive_types::NONE_TYPE()),
      agg_(aggregators::invalid_aggregator()),
      aggs_(nullptr),
      concurrency_(0) {
}

aggregate::aggregate(const data_type &type, aggregator agg, int concurrency)
    : type_(type),
      agg_(std::move(agg)),
      aggs_(new aggregate_list[concurrency]),
      concurrency_(concurrency) {
  for (int i = 0; i < concurrency_; i++)
    aggs_[i].init(type, agg_);
}

aggregate::aggregate(const aggregate &other)
    : type_(other.type_),
      agg_(other.agg_),
      aggs_(new aggregate_list[other.concurrency_]),
      concurrency_(other.concurrency_) {
  for (int i = 0; i < other.concurrency_; i++) {
    aggs_[i] = other.aggs_[i];
  }
}

aggregate::~aggregate() {
  delete[] aggs_;
}

aggregate &aggregate::operator=(const aggregate &other) {
  type_ = other.type_;
  agg_ = other.agg_;
  aggs_ = new aggregate_list[other.concurrency_];
  concurrency_ = other.concurrency_;
  for (int i = 0; i < other.concurrency_; i++) {
    aggs_[i] = other.aggs_[i];
  }
  return *this;
}

aggregate::aggregate(aggregate &&other) noexcept {
  type_ = other.type_;
  agg_ = std::move(other.agg_);
  aggs_ = other.aggs_;
  concurrency_ = other.concurrency_;
  other.aggs_ = nullptr;
}

aggregate &aggregate::operator=(aggregate &&other) noexcept {
  type_ = other.type_;
  agg_ = std::move(other.agg_);
  aggs_ = other.aggs_;
  concurrency_ = other.concurrency_;
  other.aggs_ = nullptr;
  return *this;
}

void aggregate::seq_update(int thread_id, const numeric &value, uint64_t version) {
  aggs_[thread_id].seq_update(value, version);
}

void aggregate::comb_update(int thread_id, const numeric &value, uint64_t version) {
  aggs_[thread_id].comb_update(value, version);
}

numeric aggregate::get(uint64_t version) const {
  numeric val = agg_.zero;
  for (int i = 0; i < concurrency_; i++)
    val = agg_.comb_op(val, aggs_[i].get(version));
  return val;
}

}