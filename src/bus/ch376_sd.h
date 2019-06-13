#pragma once

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <deque>
#include <vector>
#include <boost/filesystem.hpp>

#include <gtest/gtest.h>
#include <glog/logging.h>

class InterruptController;

struct LongBuffer {
  explicit LongBuffer(size_t num_bytes_needed)
      : num_bytes_needed_(num_bytes_needed){};

  void Write(uint8_t v);
  bool HasValue() const;
  uint32_t value() const;
  std::vector<uint8_t> values_;
  const size_t num_bytes_needed_;
};

// Emulate the CH376 SD/USB storage controller.
// Incomplete.
class CH376SD {
public:
  CH376SD(InterruptController *int_controller,
          const std::string &root_directory)
      : int_controller_(int_controller), root_directory_(root_directory) {}

  ~CH376SD();

  // SystemBusDevice implementation.
  void StoreByte(uint32_t addr, uint8_t v);
  uint8_t ReadByte(uint32_t addr);

private:
  void PushDirectoryListing();
  void StreamFileContents();

  InterruptController *int_controller_;

  uint8_t current_cmd_ = 0; // If command takes parameter.
  std::deque<uint8_t> out_data_;

  uint8_t int_status_;

  bool mounted_ = false;
  boost::filesystem::path root_directory_;

  struct CH376_FileInfo {
    ~CH376_FileInfo();
    bool open = false;
    std::string path;
    boost::filesystem::directory_entry entry;
    bool enumerate_mode_ = false;
    boost::filesystem::directory_iterator directory_iterator;
    struct stat statbuf;
    FILE *f = nullptr;
    std::unique_ptr<LongBuffer> byte_read_request;
    std::unique_ptr<LongBuffer> byte_seek_request;

    void Clear();
  };
  CH376_FileInfo current_file_;
};