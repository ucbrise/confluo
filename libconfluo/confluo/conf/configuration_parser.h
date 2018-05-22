#ifndef LIBCONFLUO_CONFLUO_CONF_CONFIGURATION_PARSER_H_
#define LIBCONFLUO_CONFLUO_CONF_CONFIGURATION_PARSER_H_

#include "exceptions.h"
#include "storage/ptr_aux_block.h"

namespace confluo {

class encoding_params {
 public:
  static const std::string UNENCODED;
  static const std::string LZ4;
  static const std::string ELIAS_GAMMA;
};

const std::string encoding_params::UNENCODED = "unencoded";
const std::string encoding_params::LZ4 = "lz4";
const std::string encoding_params::ELIAS_GAMMA = "elias_gamma";

class configuration_parser {

 public:

  static uint8_t to_encoding_type(const std::string& param) {
    if (param == encoding_params::UNENCODED) {
      return storage::encoding_type::D_UNENCODED;
    } else if (param == encoding_params::LZ4) {
      return storage::encoding_type::D_LZ4;
    } else if (param == encoding_params::ELIAS_GAMMA) {
      return storage::encoding_type::D_ELIAS_GAMMA;
    } else {
      THROW(illegal_state_exception, "Invalid encoding type!");
    }
  }

};

}

#endif /* LIBCONFLUO_CONFLUO_CONF_CONFIGURATION_PARSER_H_ */
