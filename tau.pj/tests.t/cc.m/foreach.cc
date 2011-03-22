// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Distributed under the terms of the GNU General Public License v2

#include <stdlib.h>

#include <iostream>
#include <vector>
using namespace std;

extern "C" {

#include "style.h"
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
  cout << (finish - start) / (double)nops << " nsecs/op\n";
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

struct Size_tSortCriterion {
  bool operator()(long a, long b) const {
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
  void operator()(T& x) const {
    cout << x << endl;
  }
};

template <class T>
class AddValue {
 private:
  T theValue;
 public:
  AddValue(const T& v) : theValue(v) {}
  void operator()(T& elem) const {
    elem += theValue;
  }
};

int main(int argc, char *argv[]) {
  Vector v;
  Uector u;
  long n = 7;
  if (argc > 1) {
    n = strtol(argv[1], NULL, 0);
  }
  for (long i = 0; i < n; ++i) {
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
  for (long i = 0; i < v.size(); ++i) {
    cout << v[i] << endl;
  }
#endif
  return 0;
}
