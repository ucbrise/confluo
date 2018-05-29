#include "archival/filter_log_archiver.h"

namespace confluo {
namespace archival {

filter_log_archiver::filter_log_archiver()
    : filter_log_archiver("", nullptr) {
}

filter_log_archiver::filter_log_archiver(const std::string &path, filter_log *filters)
    : path_(path),
      filter_archivers_(),
      filters_(filters) {
}

filter_log_archiver::~filter_log_archiver() {
  for (auto *archiver : filter_archivers_)
    delete archiver;
}

void filter_log_archiver::archive(size_t offset) {
  init_new_archivers();
  for (size_t i = 0; i < filters_->size(); i++) {
    if (filters_->at(i)->is_valid())
      filter_archivers_.at(i)->archive(offset);
  }
}

void filter_log_archiver::init_new_archivers() {
  for (size_t i = filter_archivers_.size(); i < filters_->size(); i++) {
    std::string filter_path = archival_utils::filter_archival_path(path_, i);
    file_utils::create_dir(filter_path);
    filter_archivers_.push_back(new filter_archiver(filter_path, filters_->at(i)));
  }
}

}
}