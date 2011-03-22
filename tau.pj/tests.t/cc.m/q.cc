// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Distributed under the terms of the GNU General Public License v2

#include <stdlib.h>

#include <iostream>
#include <vector>
#include <list>
using namespace std;

extern "C" {
#include "style.h"
#include "q.h"
#include "timer.h"
#include "test.h"
}

typedef vector<int> Vector;
typedef vector<string> Uector;

static inline int range(int bound) {
  return random() % bound;
}

enum { MAX_STRING = 107 };

void PrSecs(s64 start, s64 finish) {
  cout << "Total secs = " << (finish - start) / 1000000000.0 << endl;
}

void NanosecsPerOp(s64 start, s64 finish, int nops) {
  cout << (finish - start) / static_cast<double>(nops) << " nsecs/op\n";
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

struct Node_s {
  int a;
  int b;
  dqlink_t link;
};

void q1 (int n) {
  d_q h = static_init_dq(h);
  Node_s *q;
  list<Node_s> t;
  q = new Node_s[n];
  for (int i = 0; i < n; ++i) {
    q[i].a = i;
    init_qlink(q[i].link);
    enq_dq( &h, &q[i], link);
    t.push_back(q[i]);
  }
  delete[] q;
}

struct Size_tSortCriterion {
  bool operator()(s64 a, s64 b) const {
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

void print(int x) {
  cout << x << ' ';
}

template <class T>
struct Print {
  void operator()(const T& x) const {
    cout << x << endl;
  }
};

template <class T>
class AddValue {
 private:
  T theValue;
 public:
  explicit AddValue(T v) : theValue(v) {}
  void operator()(T& elem) const {
    elem += theValue;
  }
};

void v1(int n) {
  Vector v;
  Uector u;
  for (s64 i = 0; i < n; ++i) {
    v.push_back(range(n));
    u.push_back(RandString());
  }
  for_each(u.begin(), u.end(), Print<string>()); cout << endl;
  for_each(u.begin(), u.end(), AddValue<string>(".cc"));
  for_each(u.begin(), u.end(), Print<string>()); cout << endl;

  for_each(v.begin(), v.end(), Print<int>()); cout << endl;
  for_each(v.begin(), v.end(), AddValue<int>(10));
  for_each(v.begin(), v.end(), Print<int>()); cout << endl;
#if 0
  for (s64 i = 0; i < v.size(); ++i) {
    cout << v[i] << endl;
  }
#endif
}

#define Time(_f, _n) {\
  s64 start, finish;\
  start = nsecs();\
  _f(_n);\
  finish = nsecs();\
  NanosecsPerOp(start, finish, _n);\
}

int main(int argc, char *argv[]) {
  s64 n = 7;
  if (argc > 1) {
    n = strtol(argv[1], NULL, 0);
  }
//  v1(n);
  Time(q1, n);
  return 0;
}
