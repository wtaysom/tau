// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Distributed under the terms of the GNU General Public License v2

#include <iostream>
#include <vector>
using namespace std;

#include "style.h"

#include "test.h"

int main(int argc, char *argv[]) {
  vector<size_t> v;
  size_t n = 7;
  if (argc > 1) {
    n = strtol(argv[1], NULL, 0);
  }
  cout << n << '\n';
  for (size_t i = 0; i < n; ++i) {
    v.push_back(i);
  }
  for (size_t i = 0; i < v.size(); ++i) {
    test(i == v[i]);
  }
  return 0;
}
