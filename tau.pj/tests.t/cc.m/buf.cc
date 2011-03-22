// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Distributed under the terms of the GNU General Public License v2

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>

#include <iostream>
using namespace std;

#include <test.h>
#include <debug.h>
#include <eprintf.h>

enum { BUF_SIZE = 256 };

struct Buf_s {
  char b_data[BUF_SIZE];
  int  b_key;
  int  b_inuse;
};

class Dev {
  string name_;
  int fd_;
 public:
  explicit Dev(string filename);
  ~Dev();
  void fill(Buf_s *b);
  void flush(Buf_s *b);
};

Dev::Dev(string filename) {
  name_ = filename;
  fd_ = open(name_.c_str(), O_CREAT|O_RDWR, 0766);
}

Dev::~Dev() {
  close(fd_);
}

void Dev::fill(Buf_s *b) {
  int rc;
  rc = pread(fd_, b->b_data, BUF_SIZE, b->b_key * BUF_SIZE);
  if (rc < 0) fatal("pread:");
}

void Dev::flush(Buf_s *b) {
  int rc;
  rc = pwrite(fd_, b->b_data, BUF_SIZE, b->b_key * BUF_SIZE);
  if (rc < 0) fatal("pwrite:");
}

class Cache {
  Buf_s *bufs_;
  int   clock_;
  int   num_bufs_;
  Dev   dev_;
  Buf_s *lookup(int key);
  Buf_s *victim();
 public:
  Cache(string filename, int num_bufs);
  ~Cache();
  Buf_s *get_buf(int key);
  void   put_buf(Buf_s *b);
  void   clear();
  int    num_bufs() const { return num_bufs_; }
  bool   audit();
};

Cache::Cache(string filename, int num_bufs)
    : clock_(0),             // have to be initialized inorder of declaration
      num_bufs_(num_bufs),
      dev_(filename) {
  bufs_ = new Buf_s[num_bufs_];
  clear();
}

Cache::~Cache() {
  delete[] bufs_;
}

Buf_s *Cache::lookup(int key) {
  for (int i = 0; i < num_bufs_; ++i) {
    if (key == bufs_[i].b_key) return &bufs_[i];
  }
  return NULL;
}

Buf_s *Cache::victim() {
  Buf_s *b;
  do {
    ++clock_;
    if (clock_ >= num_bufs_) {
      clock_ = 0;
    }
    b = &bufs_[clock_];
  } while (b->b_inuse);
  return b;
}

Buf_s *Cache::get_buf(int key) {
  Buf_s *b = lookup(key);
  if (!b) {
    b = victim();
    b->b_key = key;
    dev_.fill(b);
  }
  ++b->b_inuse;
  return b;
}

void Cache::put_buf(Buf_s *b) {
  dev_.flush(b);
  --b->b_inuse;
}

void Cache::clear() {
  for (int i = 0; i < num_bufs_; ++i) {
    bufs_[i].b_key   = -1;
    bufs_[i].b_inuse = 0;
  }
}

bool Cache::audit() {
  if (clock_ >= num_bufs_) return false;
  return true;
}

int main(int argc, char *argv[]) {
  string cachefile = "/tmp/cache";
  int num_bufs = 10;
  if (argc > 1) {
    cachefile = argv[1];
  }
  Cache c(cachefile, num_bufs);
  Buf_s *b;

  // Test 1. Get a buffer, set a value, and put it back
  b = c.get_buf(1);
  test(b != NULL);
  test(b->b_key == 1);
  b->b_data[0] = 'A';
  c.put_buf(b);

  c.clear();

  // Test 2. Get the buffer from test 1 and check it has what we set
  // This is not a good test, because it depends on test 1.
  // This test did fail to detect that we read the block into a
  // separate buffer. Now we have to copies of the buffer in memory.
  b = c.get_buf(1);
  test(b != NULL);
  test(b->b_key == 1);
  test(b->b_data[0] == 'A');
  c.put_buf(b);
  test(c.audit());

  // Test 3. Use up all the buffers without releasing any of them
  // Can't really test for running off of end nor can we test for
  // infinite loops.
  for (int i = 0; i < c.num_bufs(); ++i) {
    b = c.get_buf(i);
    c.put_buf(b);
  }
  b = c.get_buf(c.num_bufs());
  test(b != NULL);

  test(c.audit());

  return 0;
}
