// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Distributed under the terms of the GNU General Public License v2

#include <stdlib.h>

#include <iostream>
#include <vector>
using namespace std;

#include "style.h"
#include "test.h"

typedef vector<size_t> Vector;

static inline int range(int bound) {
  return random() % bound;
}

struct Size_tSortCriterion {
  bool operator()(size_t a, size_t b) const {
    if (a & 1) {
      if (b & 1) {
        return a > b;
      } else {
        return true;
      }
    } else {
      if (b & 1) {
        return false;
      } else {
        return a < b;
      }
    }
  }
};

int main(int argc, char *argv[]) {
  Vector v;
  size_t n = 7;
  if (argc > 1) {
    n = strtol(argv[1], NULL, 0);
  }
  for (size_t i = 0; i < n; ++i) {
    v.push_back(range(i+1));
  }
  sort(v.begin(), v.end(), Size_tSortCriterion());
  for (size_t i = 0; i < v.size(); ++i) {
    cout << v[i] << endl;
  }
  return 0;
}
