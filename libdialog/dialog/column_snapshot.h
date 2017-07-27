#ifndef DIALOG_COLUMN_SNAPSHOT_H_
#define DIALOG_COLUMN_SNAPSHOT_H_

namespace dialog {

struct column_snapshot {
  data_type type;
  size_t offset;
  bool indexed;
  uint32_t index_id;
  double index_bucket_size;
};

}

#endif /* DIALOG_COLUMN_SNAPSHOT_H_ */
