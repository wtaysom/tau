// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Distributed under the terms of the GNU General Public License v2

#include <stdlib.h>

#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <cstdlib>
using namespace std;

#include "style.h"
#include "debug.h"

#include "test.h"

char *randstring() {
  char s[] = "abcdefghijklmnopqrstuvwxyz";
  static char buf[10];
  for (int i = 0; i < 10; ++i) {
    buf[i] = s[random() % sizeof s];
  }
  return buf;
}

int main(int argc, char *argv[]) {
  typedef map<size_t, string> Map;
  Map m;
  vector<string> v;
  size_t n = 7;

  if (argc > 1) {
    n = strtol(argv[1], NULL, 0);
  }
  for (size_t i = 0; i < n; ++i) {
    string x = randstring();
    m.insert(make_pair(i, x));
    v.push_back(x);
  }

  Map::const_iterator p;
  size_t i = 0;
  for (p = m.begin(); p != m.end(); ++p) {
    test(p->first == i);
    test(p->second == v[i++]);
    cout << p->second << endl;
  }
  return 0;
}
