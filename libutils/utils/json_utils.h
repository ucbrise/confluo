#ifndef UTILS_JSON_UTILS_H_
#define UTILS_JSON_UTILS_H_

#include "exceptions.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/stream_buffer.hpp>

namespace pt = boost::property_tree;

namespace utils {

class json_utils {
 public:
  static void json_to_ptree(pt::ptree& out, const std::string& json) {
    try {
      boost::iostreams::stream<boost::iostreams::array_source> stream(json.c_str(), json.size());
      pt::read_json(stream, out);
    } catch (pt::json_parser_error& e) {
      THROW(confluo::invalid_operation_exception, e.what());
    } catch (...) {
      THROW(confluo::invalid_operation_exception, "JSON format failed to read for some reason");
    }
  }
};

}

#endif /* UTILS_JSON_UTILS_H_ */
