#include "archival/index_log_archiver.h"

namespace confluo {
namespace archival {

index_log_archiver::index_log_archiver()
    : index_log_archiver("", nullptr, nullptr) {
}

index_log_archiver::index_log_archiver(const std::string &path, index_log *indexes, schema_t *schema)
    : path_(path),
      index_archivers_(),
      indexes_(indexes),
      schema_(schema) {
}

index_log_archiver::~index_log_archiver() {
  for (auto *archiver : index_archivers_)
    delete archiver;
}

void index_log_archiver::archive(size_t offset) {
  init_new_archivers();
  for (size_t i = 0; i < schema_->size(); i++) {
    auto &col = (*schema_)[i];
    if (col.is_indexed()) {
      index_archivers_.at(col.index_id())->archive(offset);
    }
  }
}

void index_log_archiver::init_new_archivers() {
  for (size_t i = 0; i < schema_->size(); i++) {
    auto &col = (*schema_)[i];
    if (col.is_indexed()) {
      auto id = col.index_id();
      if (index_archivers_.size() <= id) {
        index_archivers_.resize(id + 1);
      }
      if (index_archivers_[id] == nullptr) {
        std::string index_path = path_ + "/index_" + std::to_string(id) + "/";
        file_utils::create_dir(index_path);
        index_archivers_[id] = new index_archiver(index_path, indexes_->at(id), col);
      }
    }
  }
}

}
}