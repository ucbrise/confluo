#ifndef CONFLUO_ARCHIVAL_ENCODER_H_
#define CONFLUO_ARCHIVAL_ENCODER_H_

namespace confluo {
namespace archival {

enum encoding_type {
  IDENTITY
};

class encoder {
 public:
  typedef std::unique_ptr<uint8_t, void (*)(uint8_t*)> raw_encoded_ptr;

  /**
   * Encode pointer.
   * @param ptr unencoded data pointer, allocated by the storage allocator.
   * @param encoded_size size of encoded data in bytes
   * @return raw encoded pointer (no metadata)
   */
  template<typename T, encoding_type ENCODING>
  static raw_encoded_ptr encode(T* ptr, size_t& encoded_size) {
    encoded_size = storage::ptr_metadata::get(ptr)->data_size_;
    return raw_encoded_ptr(reinterpret_cast<uint8_t*>(ptr), no_op_delete);
  }

 private:
  static void no_op_delete(uint8_t* ptr) { }

};

}
}

#endif /* CONFLUO_ARCHIVAL_ENCODER_H_ */
