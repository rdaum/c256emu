#include "bus/ch376_sd.h"

#include <cstring>
#include <fstream>

#include "bus/int_controller.h"
#include "ch376_sd.h"

namespace {
constexpr uint32_t SDCARD_DATA = 0xE808; // (R/W) SDCARD (CH376S) Data PORT_A (A0 = 0)
constexpr uint32_t SDCARD_CMD = 0xE809; // (R/W) SDCARD (CH376S) CMD/STATUS Port (A0 = 1)
constexpr uint32_t SDCARD_STAT = 0xE810; // (R) SDCARD (Bit[0] = CD, Bit[1] = WP)

enum SdCommands {
  GET_IC_VER = 0x01,
  SET_BAUDRATE = 0x02,
  ENTER_SLEEP = 0x03,
  RESET_ALL = 0x05,
  CHECK_EXIST = 0x06,
  SET_SD0_INT = 0x0b,
  GET_FILE_SIZE = 0x0c,
  SET_USB_MODE = 0x15,
  GET_STATUS = 0x22,
  RD_USB_DATA0 = 0x27,
  WR_USB_DATA = 0x2c,
  WR_REQ_DATA = 0x2d,
  WR_OFS_DATA = 0x2e,
  SET_FILE_NAME = 0x2f,
  DISK_CONNECT = 0x30,
  DISK_MOUNT = 0x31,
  FILE_OPEN = 0x32,
  FILE_ENUM_GO = 0x33,
  FILE_CREATE = 0x34,
  FILE_ERASE = 0x35,
  FILE_CLOSE = 0x36,
  DIR_INF0_READ = 0x37,
  DIR_INF0_SAVE = 0x38,
  BYTE_LOCATE = 0x39,
  BYTE_READ = 0x3a,
  BYTE_RD_GO = 0x3b,
  BYTE_WRITE = 0x3c,
  BYTE_WR_GO = 0x3d,
  DISK_CAPACITY = 0x3e,
  DISK_QUERY = 0x3f,
  DIR_CREATE = 0x40,
  SEG_LOCATE = 0x4A,
  SEC_READ = 0x4b,
  SEC_WRITE = 0x4c,
  DISK_BOC_CMD = 0x50,
  DISK_READ = 0x54,
  DISK_RD_GO = 0x55,
  DISK_WRITE = 0x56,
  DISK_WR_GO = 0x57,
};

enum SdResponse {
  CMD_RET_SUCCESS = 0x51,
  CMD_RET_ABORT = 0x5f,
};

enum InterruptState {
  USB_INT_SUCCESS = 0x14,
  USB_INT_CONNECT = 0x15,
  USB_INT_DISCONNECT = 0x16,
  USB_INT_BUF_OVER = 0x17,
  USB_INT_USB_READY = 0x18,
  USB_INT_DISK_READ = 0x1d,
  USB_INT_DISK_WRITE = 0x1e,
  USB_INT_DISK_ERR = 0x1f,
};

void Push32(uint32_t v, std::deque<uint8_t> *out) {
  uint8_t highest_byte = v >> 24;
  uint8_t next_byte = (v & 0x00ff0000) >> 16;
  uint8_t nextest_byte = (v & 0x0000ff00) >> 8;
  uint8_t lowest_byte = (v & 0x000000ff);
  out->push_back(highest_byte);
  out->push_back(next_byte);
  out->push_back(nextest_byte);
  out->push_back(lowest_byte);
}
} // namespace

void CH376SD::StoreByte(uint32_t addr, uint8_t v) {
  LOG(INFO) << "WRITE: " << addr << " 0x" << std::hex << (int)v;

  if (addr == SDCARD_CMD) {
    current_cmd_ = 0;
    switch (v) {
    case CHECK_EXIST:
      out_data_.push_back(CMD_RET_SUCCESS);
      return;
    case SET_USB_MODE:
      current_cmd_ = v;
      return;
    case GET_STATUS:
      int_controller_->LowerCH376();
      out_data_.push_back(int_status_);
      int_status_ = 0;
      return;
    case DISK_MOUNT:
      mounted_ = true;
      int_status_ = USB_INT_SUCCESS;
      int_controller_->RaiseCH376();
      LOG(INFO) << "DISK_MOUNT";
      return;
    case SET_FILE_NAME:
      current_file_.Clear();
      current_file_.path = root_directory_;
      current_cmd_ = SET_FILE_NAME;
      return;
    case FILE_OPEN: {
      if (stat(current_file_.path.c_str(), &current_file_.statbuf) != 0) {
        int_status_ = 0x42; // ERR_MISS_FILE
        LOG(INFO) << "ERR_MISS_FILE";
        current_file_.open = false;
      } else if (current_file_.statbuf.st_mode & S_IFDIR) {
        current_file_.is_dir = true;
        int_status_ = 0x1d; // docs say ERR_OPEN_DIR but kernel expects
        // USB_INT_DISK_READ
        current_file_.dir = opendir(current_file_.path.c_str());
        while ((current_file_.dirent = readdir(current_file_.dir))) {
          if (current_file_.dirent->d_name[0] == '.')
            continue;
          LOG(INFO) << current_file_.dirent->d_name;
          break;
        }
        if (current_file_.dirent)
          current_file_.open = true;
        CHECK(current_file_.dir);
      } else {
        current_file_.open = true;
        current_file_.is_dir = false;
        current_file_.f = fopen(current_file_.path.c_str(), "r");
        if (!current_file_.f) {
          LOG(INFO) << "Unable to open file: " << current_file_.path.c_str();
          int_status_ = 0x42; // ERR_MISS_FILE
          current_file_.open = false;
          return;
        }
        int_status_ = USB_INT_SUCCESS;
        return;
      }
      current_file_.byte_seek_request.reset();
      int_controller_->RaiseCH376();
      return;
    }
    case FILE_CLOSE: {
      current_file_.open = false;
      current_cmd_ = FILE_CLOSE;
      if (current_file_.dir) {
        closedir(current_file_.dir);
      } else {
        CHECK(fclose(current_file_.f) == 0);
      }
      LOG(INFO) << "FILE CLOSE:" << current_file_.path;
      return;
    }
    case FILE_ENUM_GO:
      LOG(INFO) << "FILE_ENUM_GO!";
    repeat:
      current_file_.dirent = readdir(current_file_.dir);
      if (current_file_.dirent && current_file_.dirent->d_name[0] == '.')
        goto repeat;

      int_status_ = !current_file_.dirent ? 0x42 : USB_INT_DISK_READ;
      int_controller_->RaiseCH376();

      break;
    case RD_USB_DATA0:
      if (current_file_.open) {
        if (current_file_.enumerate_mode_ && current_file_.is_dir &&
            current_file_.dirent) {
          PushDirectoryListing();
          return;
        } else {
          StreamFileContents();
          return;
        }
      }
      break;
    case GET_FILE_SIZE:
      current_cmd_ = GET_FILE_SIZE;
      break;
    case BYTE_READ:
      current_cmd_ = BYTE_READ;
      current_file_.byte_read_request = std::make_unique<CH376_ReadLong>(2);
      break;
    case BYTE_RD_GO:
      if (ftell(current_file_.f) < current_file_.statbuf.st_size) {
        int_status_ = USB_INT_DISK_READ;
      } else {
        int_status_ = USB_INT_SUCCESS; // done reading
      }
      int_controller_->RaiseCH376();
      break;
    case BYTE_LOCATE:
      // Seek.
      current_cmd_ = BYTE_LOCATE;
      current_file_.byte_seek_request = std::make_unique<CH376_ReadLong>(4);
      break;
    default:
      LOG(INFO) << "UNHANDLED COMMAND: " << std::hex << (int)v;
      break;
    }
    return;
  };
  if (addr == SDCARD_DATA) {
    switch (current_cmd_) {
    case SET_USB_MODE:
      CHECK_EQ(v, 0x03) << "SET_USB_MODE for invalid mode (" << v << ")";
      out_data_.push_back(CMD_RET_SUCCESS);
      out_data_.push_back(0); // byte 2;
      return;
    case SET_FILE_NAME:
      if (v == 0) {
        LOG(INFO) << "SET_FILE_NAME: " << current_file_.path;
        current_cmd_ = 0;
        return;
      }
      if (v == '*') {
        current_file_.enumerate_mode_ = true;
        return;
      }
      current_file_.path.push_back(v);
      break;
    case FILE_CLOSE:
      // v? "Update or not" ?
      int_status_ = USB_INT_SUCCESS;
      int_controller_->RaiseCH376();
      break;
    case GET_FILE_SIZE:
      Push32(current_file_.statbuf.st_size, &out_data_);
      break;
    case BYTE_READ:
      current_file_.byte_read_request->Write(v);
      if (current_file_.byte_read_request->HasValue()) {
        uint32_t bytes = current_file_.byte_read_request->value();
        if (ftell(current_file_.f) + bytes > current_file_.statbuf.st_size) {
          int_status_ = USB_INT_SUCCESS; // done reading
        } else {
          int_status_ = USB_INT_DISK_READ;
        }
        int_controller_->RaiseCH376();
      }
      break;
    case BYTE_LOCATE:
      current_file_.byte_seek_request->Write(v);
      if (current_file_.byte_seek_request->HasValue()) {
        uint32_t seek_val = current_file_.byte_seek_request->value();
        // Seek end of file?
        if (seek_val == 0xffffffff ||
            seek_val >= current_file_.statbuf.st_size) {
          seek_val = current_file_.statbuf.st_size;
        }
        fseek(current_file_.f, seek_val, SEEK_SET);
        int_status_ = USB_INT_SUCCESS;
        int_controller_->RaiseCH376();
      }
    }
  }
}

uint8_t CH376SD::ReadByte(uint32_t addr) {
  LOG(INFO) << "READ: " << addr;
  if (addr == SDCARD_DATA) {
    CHECK(!out_data_.empty());
    uint8_t data = out_data_.front();
    out_data_.pop_front();
    LOG(INFO) << "Return: 0x" << std::hex << (int)data << ", " << std::dec
              << out_data_.size() << " remaining in buffer";
    return data;
  }
  if (addr == SDCARD_CMD) {
    LOG(INFO) << "Return int status: " << int_status_;
    return int_status_;
  }
  return 0;
}

void CH376SD::PushDirectoryListing() {
  LOG(INFO) << "RD_USB_DATA0 directory: " << current_file_.dirent->d_name;

  // Expect 32 bytes.
  out_data_.push_back(0x20);

  // 8.3 filename, only support the first 11 characters, first dot
  // breaks to extension.
  size_t c_num = 0;
  size_t d_namelen = strlen(current_file_.dirent->d_name);
  for (int i = 0; i < 8; i++) {
    char c = current_file_.dirent->d_name[c_num++];
    if (c == '.' || c_num > d_namelen) {
      while (i++ < 8)
        out_data_.push_back(0);
      continue;
    }
    out_data_.push_back(c);
  }
  for (int i = 0; i < 3; i++) {
    char c = current_file_.dirent->d_name[c_num++];
    if (c == 0 || c_num > d_namelen) {
      while (i++ < 3)
        out_data_.push_back(0);
    } else
      out_data_.push_back(c);
  }

  struct stat f_stat;
  stat(current_file_.dirent->d_name, &f_stat);
  if (S_ISDIR(f_stat.st_mode))
    out_data_.push_back(0x10);
  else
    out_data_.push_back(0);
  // 10 bytes reserved.
  for (int i = 0; i < 10; i++)
    out_data_.push_back(0);
  // TODO create/modify times/dates
  for (int i = 0; i < 4; i++)
    out_data_.push_back(0);
  // Cluster number not supported
  for (int i = 0; i < 2; i++)
    out_data_.push_back(0);
  // File size in bytes, 32 bits.
  size_t flen = f_stat.st_size;
  Push32(flen, &out_data_);

  // No more data after this.
  out_data_.push_back(0);
}

void CH376SD::StreamFileContents() {
  LOG(INFO) << "RD_USB_DATA0 file: " << current_file_.path;

  uint32_t total_read = 0;

  // We will only buffer up 255 bytes at a time, and force the client to ask for
  // more, since they're only reading 255 bytes right now at a time anyways
  // despite asking for 64k.
  uint32_t amount_to_retrieve =
      std::min<uint32_t>(current_file_.byte_read_request->value(), 255);

  do {
    // We do blocks of no more than 255 bytes;
    char byte_buffer[std::min<size_t>(255, amount_to_retrieve)];
    size_t read_bytes = fread(byte_buffer, 1, 255, current_file_.f);
    out_data_.push_back(read_bytes);
    if (!read_bytes) {
      LOG(INFO) << "EOF: " << total_read << " bytes of " << amount_to_retrieve
                << " requested " << out_data_.size() << " in buffer";
      return;
    }

    std::copy(byte_buffer, byte_buffer + read_bytes,
              std::back_inserter(out_data_));
    total_read += read_bytes;
  } while (!feof(current_file_.f) && total_read < amount_to_retrieve);

  LOG(INFO) << "READ: " << total_read << " bytes of " << amount_to_retrieve
            << " requested " << out_data_.size() << " in buffer"
            << " pos: " << ftell(current_file_.f) << " / "
            << current_file_.statbuf.st_size;

  // If done, put 0.
  if (feof(current_file_.f))
    out_data_.push_back(0);
}

void CH376_ReadLong::Write(uint8_t v) { values_.push_back(v); }

bool CH376_ReadLong::HasValue() const {
  return values_.size() == num_bytes_needed_;
}

uint32_t CH376_ReadLong::value() const {
  CHECK(HasValue());
  CHECK_LT(values_.size(), 5u);
  uint32_t v = 0;
  for (int i = values_.size(); i-- > 0;) {
    v |= values_[i] << (i * 8);
  }
  return v;
}

void CH376_FileInfo::Clear() {
  open = false;
  path = "";
  enumerate_mode_ = false;
  is_dir = false;
  dir = nullptr;
  dirent = nullptr;
  statbuf = {};
  f = nullptr;
  byte_read_request.reset();
  byte_seek_request.reset();
}
