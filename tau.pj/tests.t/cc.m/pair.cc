// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Distributed under the terms of the GNU General Public License v2

#include <iostream>
#include <utility>
using namespace std;

#include "test.h"

int main() {
  pair<int,int> p(1,2);
  pair<int,int> q(1,3);
  pair<int,int> r(1,1);

  test(p != q);
  test(p < q);
  test(r < p);

  return 0;
}
