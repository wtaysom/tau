// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Distributed under the terms of the GNU General Public License v2

#include <iostream>
#include <set>
using namespace std;

#include "style.h"
#include "debug.h"

#include "test.h"

int main(int argc, char *argv[]) {
  typedef set<size_t> Set;
  Set s;
  size_t n = 7;
  if (argc > 1) {
    n = strtol(argv[1], NULL, 0);
  }
  for (size_t i = 0; i < n; ++i) {
    s.insert(i);
  }

  Set::const_iterator p;
  size_t i = 0;
  for (p = s.begin(); p != s.end(); ++p) {
    test(*p == i++);
  }
  return 0;
}
