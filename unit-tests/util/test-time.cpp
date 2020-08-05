/**@file
 * This file is part of the CANopen Library Unit Test Suite.
 *
 * @copyright 2020 N7 Space Sp. z o.o.
 *
 * Unit Test Suite was developed under a programme of,
 * and funded by, the European Space Agency.
 *
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <CppUTest/TestHarness.h>

#include <lely/util/time.h>

TEST_GROUP(Util_Time) {
  timespec ts;
  timespec ts2;

  TEST_SETUP() {
    ts = {0L, 0L};
    ts2 = {0L, 0L};
  }
};

TEST(Util_Time, TimespecAddSec) {
  timespec_add_sec(&ts, 0L);
  CHECK_EQUAL(0L, ts.tv_sec);
  CHECK_EQUAL(0L, ts.tv_nsec);

  timespec_add_sec(&ts, 1L);
  CHECK_EQUAL(1L, ts.tv_sec);
  CHECK_EQUAL(0L, ts.tv_nsec);

  timespec_add_sec(&ts, 2L);
  CHECK_EQUAL(3L, ts.tv_sec);
  CHECK_EQUAL(0L, ts.tv_nsec);
}

TEST(Util_Time, TimespecAddMSec) {
  timespec_add_msec(&ts, 0L);
  CHECK_EQUAL(0L, ts.tv_sec);
  CHECK_EQUAL(0L, ts.tv_nsec);

  timespec_add_msec(&ts, 1L);
  CHECK_EQUAL(0L, ts.tv_sec);
  CHECK_EQUAL(1000000L, ts.tv_nsec);

  timespec_add_msec(&ts, 2L);
  CHECK_EQUAL(0L, ts.tv_sec);
  CHECK_EQUAL(3000000L, ts.tv_nsec);
}

TEST(Util_Time, TimespecAddUSec) {
  timespec_add_usec(&ts, 0L);
  CHECK_EQUAL(0L, ts.tv_sec);
  CHECK_EQUAL(0L, ts.tv_nsec);

  timespec_add_usec(&ts, 1L);
  CHECK_EQUAL(0L, ts.tv_sec);
  CHECK_EQUAL(1000L, ts.tv_nsec);

  timespec_add_usec(&ts, 2L);
  CHECK_EQUAL(0L, ts.tv_sec);
  CHECK_EQUAL(3000L, ts.tv_nsec);
}

TEST(Util_Time, TimespecAddNsec) {
  timespec_add_nsec(&ts, 0L);
  CHECK_EQUAL(0L, ts.tv_sec);
  CHECK_EQUAL(0L, ts.tv_nsec);

  timespec_add_nsec(&ts, 1L);
  CHECK_EQUAL(0L, ts.tv_sec);
  CHECK_EQUAL(1L, ts.tv_nsec);

  timespec_add_nsec(&ts, 2L);
  CHECK_EQUAL(0L, ts.tv_sec);
  CHECK_EQUAL(3L, ts.tv_nsec);

  timespec_add_nsec(&ts, 999999997L);
  CHECK_EQUAL(1L, ts.tv_sec);
  CHECK_EQUAL(0L, ts.tv_nsec);
}

TEST(Util_Time, TimespecSub_Zero) {
  timespec dec = {0L, 0L};

  timespec_sub(&ts, &dec);
  CHECK_EQUAL(0L, ts.tv_sec);
  CHECK_EQUAL(0L, ts.tv_nsec);
}

TEST(Util_Time, TimespecSub_OneNsec) {
  timespec_add_sec(&ts, 2L);
  timespec dec = {0L, 1L};

  timespec_sub(&ts, &dec);
  CHECK_EQUAL(1L, ts.tv_sec);
  CHECK_EQUAL(999999999L, ts.tv_nsec);
}

TEST(Util_Time, TimespecSub_OneSecOneNsec) {
  timespec_add_sec(&ts, 2L);
  timespec dec = {1L, 1L};

  timespec_sub(&ts, &dec);
  CHECK_EQUAL(0L, ts.tv_sec);
  CHECK_EQUAL(999999999L, ts.tv_nsec);
}

TEST(Util_Time, TimespecSubSec) {
  timespec_add_sec(&ts, 2L);

  timespec_sub_sec(&ts, 1L);
  CHECK_EQUAL(1L, ts.tv_sec);
  CHECK_EQUAL(0L, ts.tv_nsec);

  timespec_sub_sec(&ts, 1L);
  CHECK_EQUAL(0L, ts.tv_sec);
  CHECK_EQUAL(0L, ts.tv_nsec);
}

TEST(Util_Time, TimespecSubMsec) {
  timespec_add_sec(&ts, 2L);

  timespec_sub_msec(&ts, 0L);
  CHECK_EQUAL(2L, ts.tv_sec);
  CHECK_EQUAL(0L, ts.tv_nsec);

  timespec_sub_msec(&ts, 1L);
  CHECK_EQUAL(1L, ts.tv_sec);
  CHECK_EQUAL(999000000L, ts.tv_nsec);

  timespec_sub_msec(&ts, 999L);
  CHECK_EQUAL(1L, ts.tv_sec);
  CHECK_EQUAL(0L, ts.tv_nsec);
}

TEST(Util_Time, TimespecSubUsec) {
  timespec_add_sec(&ts, 2L);

  timespec_sub_usec(&ts, 0L);
  CHECK_EQUAL(2L, ts.tv_sec);
  CHECK_EQUAL(0L, ts.tv_nsec);

  timespec_sub_usec(&ts, 100000L);
  CHECK_EQUAL(1L, ts.tv_sec);
  CHECK_EQUAL(900000000L, ts.tv_nsec);

  timespec_sub_usec(&ts, 1900000L);
  CHECK_EQUAL(0L, ts.tv_sec);
  CHECK_EQUAL(0L, ts.tv_nsec);
}

TEST(Util_Time, TimespecSubNsec) {
  timespec_add_sec(&ts, 2L);

  timespec_sub_nsec(&ts, 0L);
  CHECK_EQUAL(2L, ts.tv_sec);
  CHECK_EQUAL(0L, ts.tv_nsec);

  timespec_sub_nsec(&ts, 100000L);
  CHECK_EQUAL(1L, ts.tv_sec);
  CHECK_EQUAL(999900000L, ts.tv_nsec);

  timespec_sub_nsec(&ts, 1999900000LL);
  CHECK_EQUAL(0L, ts.tv_sec);
  CHECK_EQUAL(0L, ts.tv_nsec);
}

TEST(Util_Time, TimespecDiffSec_Seconds) {
  CHECK_EQUAL(0L, timespec_diff_sec(&ts, &ts2));

  ts = {3L, 0L};
  ts2 = {2L, 0L};
  CHECK_EQUAL(1L, timespec_diff_sec(&ts, &ts2));

  ts = {2L, 5000000L};
  ts2 = {2L, 0L};
  CHECK_EQUAL(0L, timespec_diff_sec(&ts, &ts2));
}

TEST(Util_Time, TimespecDiffSec_Nanoseconds) {
  ts2 = {0L, 1000L};
  CHECK_EQUAL(0L, timespec_diff_sec(&ts, &ts2));

  ts2 = {0L, 999999999L};
  CHECK_EQUAL(0L, timespec_diff_sec(&ts, &ts2));
}

TEST(Util_Time, TimespecDiffMsec_Seconds) {
  ts2 = {0L, 0L};
  CHECK_EQUAL(0L, timespec_diff_msec(&ts, &ts2));

  ts2 = {1L, 0L};
  CHECK_EQUAL(-1000L, timespec_diff_msec(&ts, &ts2));

  ts = {1L, 0L};
  CHECK_EQUAL(0L, timespec_diff_msec(&ts, &ts2));

  ts = {3L, 0L};
  CHECK_EQUAL(2000L, timespec_diff_msec(&ts, &ts2));
}

TEST(Util_Time, TimespecDiffMsec_Nanoseconds) {
  ts = {0L, 2000000L};
  ts2 = {0L, 1000000L};
  CHECK_EQUAL(1L, timespec_diff_msec(&ts, &ts2));

  ts = {0, 1000000000L};
  ts2 = {0L, 1000000L};
  CHECK_EQUAL(999L, timespec_diff_msec(&ts, &ts2));

  ts = {4L, 200000000L};
  ts2 = {2L, 100000000L};
  CHECK_EQUAL(2100L, timespec_diff_msec(&ts, &ts2));
}

TEST(Util_Time, TimespecDiffUsec_Seconds) {
  ts2 = {0L, 0L};
  CHECK_EQUAL(0L, timespec_diff_usec(&ts, &ts2));

  ts2 = {1L, 0L};
  CHECK_EQUAL(-1000000L, timespec_diff_usec(&ts, &ts2));

  ts = {1L, 0L};
  CHECK_EQUAL(0L, timespec_diff_usec(&ts, &ts2));

  ts = {3L, 0L};
  CHECK_EQUAL(2000000L, timespec_diff_usec(&ts, &ts2));
}

TEST(Util_Time, TimespecDiffUsec_Nanoseconds) {
  ts = {0L, 2000000L};
  ts2 = {0L, 1000000L};
  CHECK_EQUAL(1000L, timespec_diff_usec(&ts, &ts2));

  ts = {0, 1000000L};
  ts2 = {0L, 1000L};
  CHECK_EQUAL(999L, timespec_diff_usec(&ts, &ts2));

  ts = {4L, 200000000L};
  ts2 = {2L, 1000000L};
  CHECK_EQUAL(2199000L, timespec_diff_usec(&ts, &ts2));
}

TEST(Util_Time, TimespecDiffNsec_Seconds) {
  ts2 = {0L, 0L};
  CHECK_EQUAL(0L, timespec_diff_nsec(&ts, &ts2));

  ts2 = {1L, 0L};
  CHECK_EQUAL(-1000000000L, timespec_diff_nsec(&ts, &ts2));

  ts = {1L, 0L};
  CHECK_EQUAL(0L, timespec_diff_nsec(&ts, &ts2));

  ts = {3L, 0L};
  CHECK_EQUAL(2000000000L, timespec_diff_nsec(&ts, &ts2));
}

TEST(Util_Time, TimespecDiffNsec_Nanoseconds) {
  ts = {0L, 2000000L};
  ts2 = {0L, 1000000L};
  CHECK_EQUAL(1000000L, timespec_diff_nsec(&ts, &ts2));

  ts = {0, 1000000L};
  ts2 = {0L, 1L};
  CHECK_EQUAL(999999L, timespec_diff_nsec(&ts, &ts2));

  ts = {4L, 200L};
  ts2 = {2L, 10L};
  CHECK_EQUAL(2000000190L, timespec_diff_nsec(&ts, &ts2));
}

TEST(Util_Time, TimespecCmp) {
  ts = {2L, 100L};
  ts2 = ts;

  CHECK_EQUAL(0L, timespec_cmp(&ts, &ts));
  CHECK_EQUAL(-1L, timespec_cmp(nullptr, &ts));
  CHECK_EQUAL(1L, timespec_cmp(&ts, nullptr));
  CHECK_EQUAL(0, timespec_cmp(&ts, &ts2));

  ts2 = {ts.tv_sec / 2, ts.tv_nsec / 2};
  CHECK_EQUAL(1, timespec_cmp(&ts, &ts2));
  ts2 = {2 * ts.tv_sec, 2 * ts.tv_nsec};
  CHECK_EQUAL(-1, timespec_cmp(&ts, &ts2));
}
