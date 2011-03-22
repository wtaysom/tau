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
#include "timer.h"
#include "test.h"

enum { MAX_STRING = 107 };

void PrSecs(s64 start, s64 finish) {
  cout << "Total secs = " << (finish - start) / 1000000000.0 << endl;
}

void NanosecsPerOp(s64 start, s64 finish, int nops) {
  cout << (finish - start) / (double)nops << " nsecs/op\n";
}

static inline int range(int bound) {
  return random() % bound;
}

char *RandString() {
  char s[] = "abcdefghijklmnopqrstuvwxyz";
  static char buf[MAX_STRING+1];
  int len = (range(MAX_STRING) + range(MAX_STRING) + range(MAX_STRING)) / 3;
  if (!len) len = 1;
  for (int i = 0; i < len; ++i) {
    buf[i] = s[range(sizeof s - 1)];
  }
  buf[len] = '\0';
  return buf;
}

int main(int argc, char *argv[]) {
  s64 start, finish;
  typedef map<string, string> Map;
  Map a;
  vector<string> v;
  size_t n = 7;

  if (argc > 1) {
    n = strtol(argv[1], NULL, 0);
  }
  for (size_t i = 0; i < n; ++i) {
    string x = RandString();
    string y = RandString();
    a.insert(make_pair(x, y));
    v.push_back(x);
  }
  start = nsecs();
  Map::const_iterator p;
  for (size_t i = 0; i < n; ++i) {
    p = a.find(v[i]);
    test(p->first == v[i]);
//    cout << p->second << endl;
  }
  finish = nsecs();
  PrSecs(start, finish);
  NanosecsPerOp(start, finish, n);

  Map b;
  start = nsecs();
  for (p = a.begin(); p != a.end(); ++p) {
    b.insert(*p);
  }
  finish = nsecs();
  NanosecsPerOp(start, finish, n);

  start = nsecs();
  test(a == b);
  finish = nsecs();
  NanosecsPerOp(start, finish, n);
  return 0;
}
