#ifndef DIALOG_DIALOG_ENCODER_H_
#define DIALOG_DIALOG_ENCODER_H_

namespace dialog {
namespace archival {

// TODO replace this, need common iface
template<typename T>
class encoder {

 public:
  /**
   * Encode pointer.
   * @param ptr unencoded pointer
   * @return encoded pointer
   */
  void* encode(T* ptr) {
    return ptr;
  }

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

#endif /* DIALOG_DIALOG_ENCODER_H_ */
