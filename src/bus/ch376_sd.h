#pragma once

#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <memory>
#include <sys/stat.h>
#include <sys/types.h>

#include <deque>

#include "cpu/cpu_65816.h"

class InterruptController;

struct CH376_ReadLong {
  explicit CH376_ReadLong(size_t num_bytes_needed)
      : num_bytes_needed_(num_bytes_needed){};

  void Write(uint8_t v);

  bool HasValue() const;

  uint32_t value() const;

  std::vector<uint8_t> values_;
  const size_t num_bytes_needed_;
};

struct CH376_FileInfo {
  bool open = false;
  std::string path;
  bool enumerate_mode_ = false;
  bool is_dir = false;
  DIR *dir = nullptr;
  struct dirent *dirent = nullptr;
  struct stat statbuf;
  FILE *f = nullptr;
  std::unique_ptr<CH376_ReadLong> byte_read_request;
  std::unique_ptr<CH376_ReadLong> byte_seek_request;

  void Clear();
};

// Emulate the CH376 SD/USB storage controller.
// Incomplete.
class CH376SD : public SystemBusDevice {
public:
  CH376SD(InterruptController *int_controller,
          const std::string &root_directory)
      : int_controller_(int_controller), root_directory_(root_directory) {}

  // SystemBusDevice implementation.
  void StoreByte(const Address &addr, uint8_t v, uint8_t **address) override;
  uint8_t ReadByte(const Address &addr, uint8_t **address) override;
  bool DecodeAddress(const Address &from_addr, Address &to_addr) override;

private:
  void PushDirectoryListing();
  void StreamFileContents();

  InterruptController *int_controller_;

  uint8_t current_cmd_ = 0; // If command takes parameter.
  std::deque<uint8_t> out_data_;

  uint8_t int_status_;

  bool mounted_ = false;
  std::string root_directory_;

  CH376_FileInfo current_file_;

  bool streaming_ = false;
};