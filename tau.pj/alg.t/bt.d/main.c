/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

// Needed tests:
// 1. Test for non-existant keys
// 2. Inserting keys that already exist

#include <ctype.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <crc.h>
#include <debug.h>
#include <eprintf.h>
#include <mystdlib.h>
#include <style.h>

#include "bt.h"

enum { NUM_BUFS = 10 };

struct {
  int iterations;
  int level;
  bool debug;
  bool print;
} static Option = {
  .iterations = 3000,
  .level = 150,
  .debug = FALSE,
  .print = FALSE };

char *rndstring(unint n)
{
  char *s;
  unint j;

  if (!n) return NULL;

  s = malloc(n);
  for (j = 0; j < n-1; j++) {
    s[j] = 'a' + urand(26);
  }
  s[j] = 0;
  return s;
}

Lump_s rnd_lump(void)
{
  unint n;

  n = urand(7) + 5;
  return lumpmk(n, rndstring(n));
}

Lump_s fixed_lump(unint n)
{
  return lumpmk(n, rndstring(n));
}

Lump_s seq_lump(void)
{
  enum { MAX_KEY = 4 };

  static int n = 0;
  int x = n++;
  char *s;
  int i;
  int r;

  s = malloc(MAX_KEY);
  i = MAX_KEY - 1;
  s[i]   = '\0';
  do {
    --i;
    r = x % 26;
    x /= 26;
    s[i] = 'a' + r;
  } while (x && (i > 0));
  while (--i >= 0) {
    s[i] = ' ';
  }
  return lumpmk(MAX_KEY, s);
}

void pr_cach_stats (Btree_s *t) {
  CacheStat_s cs;
  cs = t_cache_stats(t);
  printf("cache stats:\n"
         "  num bufs =%8d\n"
         "  gets     =%8lld\n"
         "  puts     =%8lld\n"
         "  hits     =%8lld\n"
         "  miss     =%8lld\n"
         "hit ratio  =%8g%%\n",
         cs.numbufs,
         cs.gets, cs.puts, cs.hits, cs.miss,
         (double)(cs.hits) / (cs.hits + cs.miss) * 100.);
}

void pr_audit (Audit_s *a) {
  printf("Audit:\n"
         "  branches:  %8lld\n"
         "  splits:    %8lld\n"
         "  leaves:    %8lld\n"
         "  records:   %8lld\n"
         "  recs/leaf: %8g\n",
         a->branches, a->splits,
         a->leaves, a->records,
         (double)a->records / a->leaves);
}

int audit (Btree_s *t) {
  Audit_s a;
  int rc = t_audit(t, &a);
  return rc;
}

void pr_lump (Lump_s a) {
  enum { MAX_PRINT = 32 };
  int i;
  int size;

  size = a.size;
  if (size > MAX_PRINT) size = MAX_PRINT;
  for (i = 0; i < size; i++) {
    if (isprint(a.d[i])) {
      putchar(a.d[i]);
    } else {
      putchar('.');
    }
  }
}

static int pr_rec (Rec_s rec, Btree_s *t, void *user) {
  u64 *recnum = user;

  printf("%4lld. ", ++*recnum);
  pr_lump(rec.key);
  printf(" = ");
  pr_lump(rec.val);
  printf("\n");
  return 0;
}

void pr_tree (Btree_s *t) {
  u64 recnum = 0;

  printf("******************************\n");
  t_map(t, pr_rec, NULL, &recnum);
}

void pr_stats (Btree_s *t) {
  Stat_s stat = t_get_stats(t);
  u64 records = stat.insert - stat.delete;

  printf("Num new leaves   = %8lld\n", stat.new_leaves);
  printf("Num new branches = %8lld\n", stat.new_branches);
  printf("Num split leaf   = %8lld\n", stat.split_leaf);
  printf("Num split branch = %8lld\n", stat.split_branch);
  printf("Num insert       = %8lld\n", stat.insert);
  printf("Num find         = %8lld\n", stat.find);
  printf("Num delete       = %8lld\n", stat.delete);
  printf("Num join         = %8lld\n", stat.join);
  printf("Num records      = %8lld\n", records);
  printf("Records per leaf = %g\n",
    (double)records / stat.new_leaves);
}

void t_print (Btree_s *t) {
  if (Option.print) {
    t_dump(t);
  }
}

enum { NUM_BUCKETS = (1 << 20) + 1,
  DYNA_START  = 1,
  DYNA_MAX    = 1 << 27 };

typedef void (*recFunc)(Lump_s key, Lump_s val, void *user);

typedef struct Record_s Record_s;
struct Record_s {
  Lump_s  key;
  Lump_s  val;
};

static Record_s *R;
static Record_s *Next;
static unint Size;

static void r_add(Lump_s key, Lump_s val)
{
  Record_s *r;

  if (!R) {
    Size = DYNA_START;
    Next = R = emalloc(Size * sizeof(*R));
  }
  if (Next == &R[Size]) {
    unint  newsize;

    if (Size >= DYNA_MAX) {
      fatal("Size %ld > %d", Size, DYNA_MAX);
    }
    newsize = Size << 2;
    R = erealloc(R, newsize * sizeof(*R));
    Next = &R[Size];
    Size = newsize;
  }
  r = Next++;
  r->key = key;
  r->val = val;
}

void r_for_each (recFunc f, void *user)
{
  Record_s *r;

  for (r = R; r < Next; r++) {
    f(r->key, r->val, user);
  }
}

static snint r_rand_index (void)
{
  snint x = Next - R;

  if (!R) return -1;
  if (x == 0) return -1;
  return urand(x);
}

Record_s r_get_rand (void)
{
  snint x = r_rand_index();

  if (x == -1) {
    Record_s r = { Nil, Nil };
    return r;
  }
  return R[x];
}

Lump_s r_delete_rand (void)
{
  snint x = r_rand_index();
  Lump_s key;

  if (x == -1) return Nil;
  key = R[x].key;
  R[x] = *--Next;
  return key;
}

void test1(int n)
{
  Lump_s key;
  unint i;

  for (i = 0; i < n; i++) {
    key = seq_lump();
    printf("%s\n", key.d);
    freelump(key);
  }
}

void test_seq(int n)
{
  Btree_s *t;
  Lump_s key;
  Lump_s val;
  unint i;

  if (FALSE) seed_random();
  t = t_new(".tfile", NUM_BUFS);
  for (i = 0; i < n; i++) {
    key = seq_lump();
    val = rnd_lump();
    t_insert(t, key, val);
    freelump(key);
    freelump(val);
  }
  t_print(t);
}

void test_rnd(int n)
{
  Btree_s *t;
  Lump_s key;
  Lump_s val;
  unint i;

  if (FALSE) seed_random();
  t = t_new(".tfile", NUM_BUFS);
  for (i = 0; i < n; i++) {
    key = fixed_lump(7);
    val = rnd_lump();
    t_insert(t, key, val);
    freelump(key);
    freelump(val);
  }
// t_print(t);
// pr_all_records(t);
// pr_tree(t);
  audit(t);
  pr_stats(t);
}

void find_find (Lump_s key, Lump_s val, void *user) {
  Btree_s *t = user;
  Lump_s fval;
  int rc;

  rc = t_find(t, key, &fval);
  if (rc) {
    fatal("Didn't find %s : rc=%d", key.d, rc);
  } else if (cmplump(val, fval) != 0) {
    fatal("Val not the same %s!=%s", val.d, fval.d);
  } else {
    printf("%s:%s\n", key.d, fval.d);
  }
  freelump(fval);
}

void test_find(int n)
{
  Btree_s *t;
  Lump_s key;
  Lump_s val;
  unint i;

  if (FALSE) seed_random();
  t = t_new(".tfile", NUM_BUFS);
  for (i = 0; i < n; i++) {
    key = fixed_lump(7);
    val = rnd_lump();
    r_add(key, val);
    t_insert(t, key, val);
  }
  r_for_each(find_find, t);
  audit(t);
  pr_stats(t);
}

void delete (Lump_s key, Lump_s val, void *user) {
  Btree_s *t = user;
  int rc;

  rc = t_delete(t, key);
  if (rc) {
    fatal("Didn't find %s : rc=%d", key.d, rc);
  }
}

void test_delete(int n)
{
  Btree_s *t;
  Lump_s key;
  Lump_s val;
  unint i;

  if (FALSE) seed_random();
  t = t_new(".tfile", NUM_BUFS);
  for (i = 0; i < n; i++) {
    key = fixed_lump(7);
    val = rnd_lump();
    r_add(key, val);
    t_insert(t, key, val);
  }
  r_for_each(delete, t);
  audit(t);
  pr_stats(t);
}

int should_delete(s64 count, s64 level)
{
  enum { RANGE = 1<<20, MASK = (2*RANGE) - 1 };
  return (random() & MASK) * count / level / RANGE;
}

#if 0
if (i >= 595) {
  fdebugon();
  Option.debug = TRUE;
  Option.print = TRUE;
printf("=========%d======\n", i);
}
if (i > 596) {
  fdebugoff();
  Option.debug = FALSE;
  Option.print = FALSE;
}
if (Option.print) {
  printf("delete\n");
  PRlp(key);
  t_print(t);
}
#endif

void test_level(int n, int level)
{
  Btree_s *t;
  Lump_s key;
  Lump_s val;
  s64 count = 0;
  int i;
  int rc;

  if (FALSE) seed_random();
  t = t_new(".tfile", NUM_BUFS);
  for (i = 0; i < n; i++) {
if (i % 1000 == 0) fprintf(stderr, ".");
if (i >= 298048) {
  fdebugon();
  Option.debug = TRUE;
  Option.print = TRUE;
  printf("=========%d======\n", i);
  t_print(t);
}
    if (should_delete(count, level)) {
      key = r_delete_rand();
      rc = t_delete(t, key);
      if (rc) fatal("delete key=%s", key.d);
      --count;
    } else {
      key = fixed_lump(7);
      val = rnd_lump();
      r_add(key, val);
      rc = t_insert(t, key, val);
      if (rc) fatal("t_insert key=%s val=%s",
          key.d, val.d);
      ++count;
    }
    audit(t);
  }
  audit(t);
  t_print(t);
  pr_stats(t);
  {
    Audit_s a;
    t_audit(t, &a);
    pr_audit(&a);
  }
  pr_cach_stats(t);
}

void usage(void)
{
  pr_usage("[-dhp] [-i<iterations>] [-l<level>]\n"
    "\t-d - turn on debugging [%s]\n"
    "\t-h - print this help message\n"
    "\t-i - num iterations [%d]\n"
    "\t-l - level of records to keep [%d]\n"
    "\t-p - turn on printing [%s]\n",
    Option.debug ? "on" : "off",
    Option.iterations,
    Option.level,
    Option.print ? "on" : "off");
}

void myoptions(int argc, char *argv[])
{
  int c;

  fdebugoff();
  setprogname(argv[0]);
  setlocale(LC_NUMERIC, "en_US");
  while ((c = getopt(argc, argv, "dhpi:l:")) != -1) {
    switch (c) {
    case 'h':
    case '?':
      usage();
      break;
    case 'd':
      Option.debug = TRUE;
      fdebugon();
      break;
    case 'i':
      Option.iterations = strtoll(optarg, NULL, 0);
      break;
    case 'l':
      Option.level = strtoll(optarg, NULL, 0);
      break;
    case 'p':
      Option.print = TRUE;
      break;
    default:
      fatal("unexpected option %c", c);
      break;
    }
  }
}

int main(int argc, char *argv[])
{
  myoptions(argc, argv);
  test_level(Option.iterations, Option.level);
  return 0;
}

#if 0
 test_delete(Option.iterations);
  test_level(Option.iterations, Option.level);
#endif


:2155:bt.d$ bt -i298049
...........................................................................................................................................................................................................................................................................................................=========298048======
**************************************************
  BRANCH 44 num_recs 4 free 28 end 43 last 14116
  0. hyoqqk = 15025
      BRANCH 15025 num_recs 4 free 28 end 43 last 15092
      0. bcgblb = 14627
          BRANCH 14627 num_recs 5 free 9 end 43 last 15100
          0. afnzjf = 15047
            LEAF 15047 num_recs 3 free 42 end 35 last 0
            0. adqxix = ntswc
            1. aeuhrg = kdsqieq
            2. afnzjf = pbyterck
          1. ajzyuy = 15112
            LEAF 15112 num_recs 2 free 62 end 90 last 0
            0. ahcrjf = vvlbivglap
            1. ahnvai = irkl
          2. aqixxi = 15122
            LEAF 15122 num_recs 2 free 60 end 72 last 0
            0. aknuml = xswqilchh
            1. aqixxi = rxtpdcp
          3. aufvaa = 15114
            LEAF 15114 num_recs 1 free 80 end 68 last 0
            0. atwmxs = xgrmuuuehx
          4. baurbe = 15057
            LEAF 15057 num_recs 1 free 81 end 71 last 0
            0. ayazmm = oomqhbjmm
          5. <last> = 15100
            LEAF 15100 num_recs 0 free 104 end 72 last 0
      1. chrfws = 15113
          BRANCH 15113 num_recs 4 free 28 end 60 last 15129
          0. bdwfvf = 15106
            LEAF 15106 num_recs 1 free 80 end 46 last 0
            0. bcllvv = zbnxbkzbyv
          1. bhclae = 15136
            LEAF 15136 num_recs 3 free 40 end 70 last 0
            0. bftssj = ngpyzrzsyn
            1. bfwlpi = viozfvdi
            2. bgpsru = ayqc
          2. budxpi = 15077
            LEAF 15077 num_recs 2 free 60 end 72 last 0
            0. bhunyk = dkhgueqeq
            1. bnyytw = gdaoaia
          3. bzresk = 15125
            LEAF 15125 num_recs 2 free 57 end 50 last 0
            0. byaptc = wqamvlxev
            1. bzresk = vzwrdcmhvv
          4. <last> = 15129
            LEAF 15129 num_recs 2 free 59 end 87 last 0
            0. cgkiio = tqccyzw
            1. chblkn = wekgivrixy
      2. ffqrwv = 15072
          BRANCH 15072 num_recs 4 free 28 end 60 last 15102
          0. cxtecd = 15036
            LEAF 15036 num_recs 3 free 45 end 38 last 0
            0. cimgvf = jlug
            1. clraua = rmcjhys
            2. csvicm = vnztqo
          1. ebxwcm = 15066
            LEAF 15066 num_recs 2 free 57 end 85 last 0
            0. djknla = zulqadzmp
            1. dweijd = ollztyrskq
          2. enxioy = 15055
            LEAF 15055 num_recs 1 free 82 end 89 last 0
            0. edibjf = ncxtnnws
          3. fatdmb = 14881
            LEAF 14881 num_recs 1 free 84 end 89 last 0
            0. exookt = ramnzz
          4. <last> = 15102
            LEAF 15102 num_recs 0 free 104 end 111 last 0
      3. fzjqos = 14576
          BRANCH 14576 num_recs 2 free 66 end 77 last 14854
          0. fmhixn = 15014
            LEAF 15014 num_recs 3 free 38 end 68 last 0
            0. fgpqnw = bdiwwhday
            1. fiemgg = iktrie
            2. fiwflt = zubiulclt
          1. fubazx = 14930
            LEAF 14930 num_recs 4 free 30 end 62 last 0
            0. fmxrxa = xvmk
            1. foqslj = jlel
            2. fstufy = pzqfvq
            3. fubazx = bsqs
          2. <last> = 14854
            LEAF 14854 num_recs 2 free 63 end 70 last 0
            0. fxpnqk = nexbutz
            1. fxrlja = xooptr
      4. <last> = 15092
          BRANCH 15092 num_recs 5 free 9 end 43 last 15135
          0. givzei = 14926
            LEAF 14926 num_recs 2 free 61 end 50 last 0
            0. gbfabc = qqjypikzd
            1. gbfrmh = xlqwsr
          1. gpynxf = 15086
            LEAF 15086 num_recs 4 free 24 end 56 last 0
            0. gkmevd = wcaekc
            1. glicdc = qdvdu
            2. glsvou = munf
            3. goctqx = xnukyijlo
          2. gwycqb = 15126
            LEAF 15126 num_recs 3 free 37 end 67 last 0
            0. gstmng = qxjadyro
            1. gtqwvg = arbfgqjjbw
            2. gvnijt = wcgtghz
          3. hhlsdx = 15043
            LEAF 15043 num_recs 2 free 59 end 66 last 0
            0. haruso = obuiqbzic
            1. hdzkho = rwptixwb
          4. hqixnu = 15091
            LEAF 15091 num_recs 3 free 38 end 68 last 0
            0. hiirdb = edofeclseo
            1. hlerla = reul
            2. hnhikh = dtomdngyho
          5. <last> = 15135
            LEAF 15135 num_recs 2 free 58 end 68 last 0
            0. hqpnlo = netpjjqofv
            1. hwyxrt = wvxfcdgd
  1. nebcxy = 13850
      BRANCH 13850 num_recs 2 free 66 end 77 last 15009
      0. jytugm = 14978
          BRANCH 14978 num_recs 5 free 9 end 43 last 15127
          0. ikzjjo = 15067
            LEAF 15067 num_recs 2 free 59 end 87 last 0
            0. hypvyy = myitqtbc
            1. hzkudr = tuxjrageu
          1. irvccn = 14895
            LEAF *split* 14895 num_recs 3 free 40 end 70 last 15133
            0. ilxsby = xusgeist
            1. ipnhqq = pmpylqpvw
            2. ipxozv = mohzs
            Leaf Split:
            LEAF 15133 num_recs 2 free 58 end 86 last 0
            0. iqamzx = dbaykjwz
            1. irnrrg = ysmswdlxqf
          2. jaiqik = 15111
            LEAF 15111 num_recs 3 free 39 end 69 last 0
            0. iseatf = vgvebwpej
            1. iurlbx = mgkkiy
            2. ixqrvh = hvhtbbap
          3. jnucqi = 14974
            LEAF 14974 num_recs 1 free 80 end 90 last 0
            0. jjfgzb = vdcojfjibn
          4. joqdkj = 15081
            LEAF 15081 num_recs 2 free 60 end 50 last 0
            0. jomcxc = qimmkxls
            1. joqdkj = pwnxlhye
          5. <last> = 15127
            LEAF 15127 num_recs 3 free 42 end 72 last 0
            0. jrrxrb = vqqdlw
            1. judiwp = ogoosn
            2. jwmhfg = dwumzwhm
      1. lqbnyb = 14320
          BRANCH 14320 num_recs 3 free 47 end 43 last 15056
          0. kjkqgo = 15094
            LEAF 15094 num_recs 2 free 60 end 88 last 0
            0. kgywme = mwvafgnze
            1. kixyyx = cnbifoj
          1. lcjuph = 15083
            LEAF 15083 num_recs 2 free 60 end 88 last 0
            0. knuywk = fhajcfhqd
            1. ktwjgj = lpbbyqi
          2. ljfdpd = 15098
            LEAF 15098 num_recs 1 free 84 end 89 last 0
            0. ldjpni = zymqme
          3. <last> = 15056
            LEAF 15056 num_recs 1 free 86 end 112 last 0
            0. lkuoov = oqqp
      2. <last> = 15009
          BRANCH 15009 num_recs 2 free 66 end 77 last 14933
          0. mcbksq = 15073
            LEAF 15073 num_recs 1 free 86 end 71 last 0
            0. mbajmc = prdl
          1. msxtkw = 14948
            LEAF *split* 14948 num_recs 4 free 24 end 56 last 15140
            0. mfmiwi = zdqefn
            1. mjoqsf = jajlyb
            2. mkfpcc = weohzf
            3. mnarmh = pfurdo
            Leaf Split:
            LEAF 15140 num_recs 2 free 65 end 93 last 0
            0. mnkkmb = yzbvge
            1. msupui = nobpz
          2. <last> = 14933
            LEAF 14933 num_recs 2 free 60 end 88 last 0
            0. mwfwji = wkxqtwi
            1. ncvivx = uqgdmwfmm
  2. sbgllg = 14968
      BRANCH 14968 num_recs 3 free 47 end 77 last 14965
      0. oooabi = 14939
          BRANCH 14939 num_recs 3 free 47 end 60 last 14976
          0. nsicgj = 15079
            LEAF 15079 num_recs 2 free 66 end 78 last 0
            0. nklclx = wsat
            1. noozwo = ygdoif
          1. obzijd = 15121
            LEAF 15121 num_recs 1 free 80 end 44 last 0
            0. nywvzi = kokcqsztyv
          2. ogeffw = 15097
            LEAF 15097 num_recs 2 free 62 end 69 last 0
            0. oddsnj = ttcxu
            1. ofocbj = grrfvtpvk
          3. <last> = 14976
            LEAF 14976 num_recs 0 free 104 end 75 last 0
      1. peuqet = 15068
          BRANCH 15068 num_recs 2 free 66 end 60 last 15089
          0. ouqkcp = 15076
            LEAF 15076 num_recs 1 free 86 end 54 last 0
            0. orcqmc = okev
          1. pagito = 15116
            LEAF 15116 num_recs 1 free 83 end 72 last 0
            0. ozcqpg = ibdrhej
          2. <last> = 15089
            LEAF 15089 num_recs 0 free 104 end 86 last 0
      2. qbhmhu = 15130
          BRANCH 15130 num_recs 3 free 47 end 77 last 15082
          0. prpkor = 15039
            LEAF 15039 num_recs 4 free 20 end 52 last 0
            0. phytkg = ioulmwnwm
            1. pkelpe = eunbht
            2. pqldmg = lbjbwjuev
            3. prpkor = pssi
          1. przros = 15138
            LEAF 15138 num_recs 1 free 81 end 87 last 0
            0. przros = fqficbdxh
          2. pvkxwv = 15120
            LEAF 15120 num_recs 3 free 47 end 77 last 0
            0. psxfds = jafb
            1. ptumwo = zsgyojs
            2. pvjyjv = iwdb
          3. <last> = 15082
            LEAF 15082 num_recs 2 free 66 end 37 last 0
            0. pvsxih = geypiq
            1. qbfzfj = ryzp
      3. <last> = 14965
          BRANCH 14965 num_recs 2 free 66 end 60 last 14931
          0. qyaqsw = 15049
            LEAF *split* 15049 num_recs 4 free 29 end 40 last 15139
            0. qrighr = hdetl
            1. qrzaau = epub
            2. qsksgi = wpwbdt
            3. qtprbd = btrd
            Leaf Split:
            LEAF 15139 num_recs 2 free 63 end 91 last 0
            0. qwmssh = urqpfgl
            1. qyaqsw = jydqjz
          1. rmesko = 15051
            LEAF 15051 num_recs 1 free 86 end 39 last 0
            0. rkdwbl = wsvu
          2. <last> = 14931
            LEAF 14931 num_recs 1 free 83 end 74 last 0
            0. ruqmeg = zpiohil
  3. xfpizf = 14237
      BRANCH 14237 num_recs 2 free 66 end 94 last 15119
      0. tqscqa = 14967
          BRANCH 14967 num_recs 3 free 47 end 77 last 15132
          0. slpnon = 15004
            LEAF 15004 num_recs 3 free 43 end 57 last 0
            0. sgnlqf = qnfxsu
            1. shezwi = pyloq
            2. siytvl = kznnnddc
          1. syrlvh = 15085
            LEAF 15085 num_recs 1 free 80 end 65 last 0
            0. sppxak = bydirqpfwk
          2. thjgtu = 14937
            LEAF 14937 num_recs 4 free 15 end 47 last 0
            0. taecde = aohiut
            1. tezytu = stkpyagtgf
            2. tfkjun = cogrljf
            3. thjgtu = lrlgeiatqr
          3. <last> = 15132
            LEAF 15132 num_recs 3 free 41 end 53 last 0
            0. tjgprx = psyvnk
            1. tophpn = tmewcobt
            2. tqaliv = noulxjl
      1. vdgxmc = 15015
          BRANCH 15015 num_recs 4 free 28 end 60 last 15124
          0. tuyxim = 14994
            LEAF 14994 num_recs 3 free 48 end 78 last 0
            0. traitc = pkzpc
            1. tstmjo = gvlev
            2. ttlrob = hand
          1. ulyihn = 14708
            LEAF 14708 num_recs 3 free 41 end 71 last 0
            0. txvpnx = yqcynihlr
            1. ukcpuv = tzupnty
            2. ulyihn = silwr
          2. urxifs = 15118
            LEAF 15118 num_recs 2 free 62 end 90 last 0
            0. umugxx = mzot
            1. upfmoq = awayokqzac
          3. uxelph = 15103
            LEAF 15103 num_recs 2 free 64 end 92 last 0
            0. uuvpws = clymn
            1. uweiad = tuvhgff
          4. <last> = 15124
            LEAF 15124 num_recs 0 free 104 end 91 last 0
      2. <last> = 15119
          BRANCH 15119 num_recs 2 free 66 end 94 last 15117
          0. vleety = 15005
            LEAF 15005 num_recs 2 free 68 end 96 last 0
            0. vghymc = trdq
            1. viqdwg = hjgg
          1. wccgbp = 15110
            LEAF 15110 num_recs 1 free 80 end 64 last 0
            0. vrtlmm = vbtmavqbwx
          2. <last> = 15117
              BRANCH 15117 num_recs 2 free 66 end 77 last 14959
              0. wkhhrq = 15134
                LEAF 15134 num_recs 3 free 36 end 66 last 0
                0. wgmzpi = pnmuireqgk
                1. wjmxth = pqnjfymrm
                2. wkhhrq = mijdodt
              1. wmvcfl = 15115
                LEAF 15115 num_recs 1 free 84 end 57 last 0
                0. wkkpeq = hrdhsj
              2. <last> = 14959
                LEAF 14959 num_recs 3 free 42 end 72 last 0
                0. wudzdg = ehmu
                1. xayeri = otgkgewrw
                2. xedxou = waurgbu
  4. <last> = 14116
      BRANCH 14116 num_recs 1 free 85 end 111 last 14872
      0. yiynen = 14805
          BRANCH 14805 num_recs 3 free 47 end 60 last 15070
          0. xkrbbt = 15107
            LEAF 15107 num_recs 1 free 80 end 70 last 0
            0. xkoron = hxhpyuchsa
          1. xwnuco = 15050
            LEAF 15050 num_recs 1 free 81 end 86 last 0
            0. xnezch = aapvmamlc
          2. ydunqp = 15108
            LEAF 15108 num_recs 2 free 63 end 71 last 0
            0. ybfszn = dyaj
            1. ydgyhj = xjvxjwdkr
          3. <last> = 15070
            LEAF 15070 num_recs 0 free 104 end 89 last 0
      1. <last> = 14872
          BRANCH 14872 num_recs 4 free 28 end 60 last 15071
          0. ywaamw = 15030
            LEAF 15030 num_recs 3 free 43 end 53 last 0
            0. ypfoyg = mudoz
            1. yrbrmr = sxnsrm
            2. ywaamw = gwhqzqsf
          1. zbzqst = 15137
            LEAF 15137 num_recs 2 free 67 end 78 last 0
            0. yzhbdk = izmh
            1. zarlme = sncdo
          2. zivslc = 14667
            LEAF 14667 num_recs 1 free 85 end 69 last 0
            0. zhgoit = oxxge
          3. zqoucn = 15069
            LEAF 15069 num_recs 1 free 86 end 91 last 0
            0. zqoucn = iblu
          4. <last> = 15071
            LEAF 15071 num_recs 3 free 45 end 53 last 0
            0. zqwobx = yshshsn
            1. ztysdc = gnnfh
            2. zuutih = sdnkl
t_delete
buf_get
lookup
br_delete
find_le
get_block
buf_get
lookup
victim
dev_fill
buf_put
br_delete
find_le
buf_get
lookup
victim
dev_fill
buf_put
br_delete
find_le
get_block
buf_get
lookup
victim
dev_fill
rebalance
buf_get
lookup
victim
dev_fill
join
lf_compact
usable
init_head
ASSERT FAILED: bt<337>  (head->kind == LEAF)
Stack back trace:
	bt(stacktrace+0x1a) [0x409beb]
	bt(assertError+0x29) [0x40881c]
	bt(get_rec+0x2b) [0x404166]
	bt(rec_copy+0x19) [0x4041f9]
	bt(lf_compact+0x7b) [0x4042a7]
	bt(join+0x65) [0x405661]
	bt(rebalance+0x6b) [0x4067a8]
	bt(br_delete+0x97) [0x406bf1]
	bt(t_delete+0x8e) [0x406cc6]
	bt(test_level+0xe3) [0x407f68]
	bt(main+0x1a) [0x4080a3]
	/lib/libc.so.6(__libc_start_main+0xfd) [0x7f7dc703bc4d]
	bt() [0x4035b9]

