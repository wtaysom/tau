// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Distributed under the terms of the GNU General Public License v2

#include <iostream>
#include <map>
#include <vector>
#include <cstdlib>
using namespace std;

#include <style.h>
#include <debug.h>

#include <test.h>

int main (int argc, char *argv[]) {
  typedef map<size_t, size_t> Map;
  Map m;
  vector<size_t> v;
  size_t n = 7;
  if (argc > 1) {
    n = atoi(argv[1]);
  }
  for (size_t i = 0; i < n; ++i) {
    size_t x = rand();
    m.insert(make_pair(i, x));
    v.push_back(x);
  }

  Map::const_iterator p;
  size_t i = 0;
  for (p = m.begin(); p != m.end(); ++p) {
    test(p->first == i);
    test(p->second == v[i++]);
  }
  return 0;
}
