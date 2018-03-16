#ifndef CONFLUO_ARCHIVAL_ARCHIVAL_MODE_H_
#define CONFLUO_ARCHIVAL_ARCHIVAL_MODE_H_

namespace confluo {
namespace archival {

/**
 * Describes whether or not an atomic_multilog
 * has periodic archival enabled.
 */
enum archival_mode {
  OFF = 0,
  ON = 1
};

}
}

#endif /* CONFLUO_ARCHIVAL_ARCHIVAL_MODE_H_ */
