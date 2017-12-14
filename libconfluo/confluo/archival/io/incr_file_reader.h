#ifndef CONFLUO_ARCHIVAL_INCR_FILE_READER_H_
#define CONFLUO_ARCHIVAL_INCR_FILE_READER_H_

#include "exceptions.h"
#include "incr_file_offset.h"
#include "file_utils.h"
#include "io_utils.h"

namespace confluo {
namespace archival {

class incremental_file_reader {

 public:
  incremental_file_reader(const std::string& path, const std::string& file_prefix,
                          const std::string& file_suffix)
     : file_num_(0),
       dir_path_(path),
       file_prefix_(file_prefix),
       file_suffix_(file_suffix) {
    std::ifstream metadata_ifs(metadata_path());
    last_file_num_ = io_utils::read<size_t>(metadata_ifs);
  }

  template<typename T>
  incremental_file_offset advance(size_t len) {
    if (cur_ifs_.eof())
      open_next();
    else if (tell().offset() + len * sizeof(T)  > eof_offset())
      THROW(illegal_state_exception, "Stream processed incorrectly!");
    cur_ifs_.seekg(len * sizeof(T), std::ios::cur);
    return tell();
  }

  template<typename T>
  T read() {
    if (cur_ifs_.eof())
      open_next();
    else if (tell().offset() + sizeof(T) > eof_offset())
      THROW(illegal_state_exception, "Stream processed incorrectly!");
    return utils::io_utils::read<T>(cur_ifs_);
  }

  std::string read(size_t len) {
    if (cur_ifs_.eof())
      open_next();
    else if (tell().offset() + len > eof_offset())
      THROW(illegal_state_exception, "Stream processed incorrectly!");
    return utils::io_utils::read(cur_ifs_, len);
  }

  template<typename T>
  T read_metadata() {
    std::ifstream metadata_ifs(metadata_path());
    metadata_ifs.seekg(METADATA_OFFSET);
    return utils::io_utils::read<T>(metadata_ifs);
  }

  std::string cur_path() const {
    return dir_path_ + "/" + file_prefix_ + "_" + std::to_string(file_num_) + file_suffix_;
  }

  std::string metadata_path() const {
    return dir_path_ + "/" + file_prefix_ + "_metadata" + file_suffix_;
  }

  incremental_file_offset tell() {
    // TODO handle EOF
    return incremental_file_offset(cur_path(), cur_ifs_.tellg());
  }

  bool has_more() const {
    if (!cur_ifs_.eof())
      return true;
    return file_num_ < last_file_num_;
  }

  void open() {
    cur_ifs_.open(cur_path());
  }

  void close() {
    cur_ifs_.close();
  }

 private:
  size_t eof_offset() {
    size_t cur_off = cur_ifs_.tellg();
    cur_ifs_.seekg(0, std::ios::end);
    size_t end_off = cur_ifs_.tellg();
    cur_ifs_.seekg(cur_off);
    return end_off;
  }

  void open_next() {
    cur_ifs_.close();
    file_num_++;
    cur_ifs_.open(cur_path());
  }

  std::ifstream cur_ifs_;

  size_t file_num_;
  size_t last_file_num_;
  std::string dir_path_;
  std::string file_prefix_;
  std::string file_suffix_;

  static const size_t METADATA_OFFSET = sizeof(size_t);

};

}
}

#endif /* CONFLUO_ARCHIVAL_INCR_FILE_READER_H_ */
