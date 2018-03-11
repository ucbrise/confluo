#ifndef CONFLUO_ARCHIVAL_ARCHIVER_H_
#define CONFLUO_ARCHIVAL_ARCHIVER_H_

class archiver {

 public:
  /**
   * Archive up to data log offset.
   * @param offset data log offset
   */
  virtual ~archiver() { };

  virtual void archive(size_t offset) = 0;

};

#endif /* CONFLUO_ARCHIVAL_ARCHIVER_H_ */
