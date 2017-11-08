#ifndef CONFLUO_ENCODER_H_
#define CONFLUO_ENCODER_H_

namespace confluo {
namespace archival {

// TODO replace this, need better iface since there's overlap with encoded_ptr
template<typename T>
class encoder {

 public:
  /**
   * Encode pointer.
   * @param ptr unencoded pointer
   * @return encoded pointer
   */
  uint8_t* encode(T* ptr) {
    return reinterpret_cast<uint8_t*>(ptr);
  }

  // Probably a better way to do this
  /**
   * Get the size of the encoded pointer for an
   * unencoded pointer of a particular size.
   * @param unencoded_size unencoded size in bytes
   * @return encoded size in bytes
   */
  size_t encoding_size(size_t unencoded_size) {
    return unencoded_size;
  }

};

}
}

#endif /* CONFLUO_ENCODER_H_ */
