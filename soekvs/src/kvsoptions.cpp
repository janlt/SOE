#include "end_rec.hpp"
#include "image.hpp"
#include "sandbox.hpp"

#include "equalcomp.hpp"
#include "configur.hpp"

namespace LzKVStore {

OpenOptions::OpenOptions()
    : comparator(BytewiseEqualcomp()),
      create_if_missing(false),
      error_if_exists(false),
      full_checks(false),
      env(Configuration::Default()),
      info_log(NULL),
      write_buffer_size(4<<20),
      max_open_files(1000),
      block_cache(NULL),
      block_size(4096),
      block_restart_interval(16),
      max_file_size(2<<20),
      compression(kSnappyCompression),
      reuse_logs(false),
      filter_policy(NULL)
{
}

}  // namespace LzKVStore
