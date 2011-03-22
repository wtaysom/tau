// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Distributed under the terms of the GNU General Public License v2

#include <iostream>
#include <deque>
using namespace std;

#include "style.h"
#include "debug.h"

#include "test.h"

int main(int argc, char *argv[]) {
  deque<size_t> d;
  size_t n = 7;
  if (argc > 1) {
    n = strtol(argv[1], NULL, 0);
  }
  for (size_t i = 0; i < n; ++i) {
    if (i & 1) {
      d.push_back(i);
    } else {
      d.push_front(i);
    }
  }

  size_t m = d.size() / 2 + (d.size() & 1);
  for (size_t i = 0; i < d.size(); ++i) {
    if (i & 1) {
      test(i == d[m+i/2]);
    } else {
      test(i == d[m-i/2-1]);
    }
  }
  return 0;
}
