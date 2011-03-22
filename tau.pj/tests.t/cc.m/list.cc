// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Distributed under the terms of the GNU General Public License v2

#include <iostream>
#include <deque>
#include <list>
//#include <algorithm>
#include <iterator>
using namespace std;

#include "style.h"
#include "debug.h"

#include "test.h"

typedef list<size_t> List;

List NewList(size_t n) {
  List *t = new List;
  for (size_t i = 0; i < n; ++i) {
    t->push_back(i);
  }
  return *t;
}

void PrList(List t, const char *label) {
  if (label) {
    cout << label;
  } else {
    cout << "list: ";
  }
  copy(t.begin(), t.end(), ostream_iterator<size_t>(cout, " "));
  cout << endl;
}

int main(int argc, char *argv[]) {
  List t;
  deque<size_t> d;
  size_t n = 7;
  if (argc > 1) {
    n = strtol(argv[1], NULL, 0);
  }
  for (size_t i = 0; i < n; ++i) {
    if (i & 1) {
      t.push_back(i);
      d.push_back(i);
    } else {
      t.push_front(i);
      d.push_front(i);
    }
  }

  deque<size_t>::const_iterator p;
  for (p = d.begin(); p != d.end(); ++p) {
    test(*p == t.front());
    t.pop_front();
  }
  test(t.empty());

  List u = NewList(8);
  PrList(u, NULL);
  remove(u.begin(), u.end(), 3);
  PrList(u, "post: ");

  return 0;
}
