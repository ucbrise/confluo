#ifndef DIALOG_RECORD_BATCH_H_
#define DIALOG_RECORD_BATCH_H_

namespace dialog {

struct record_block {
  uint64_t time_block;
  void* data;
  size_t nrecords;
};

struct record_batch {
  record_block* blocks;
  size_t nblocks;
  size_t nrecords;
};

}

#endif /* DIALOG_RECORD_BATCH_H_ */
