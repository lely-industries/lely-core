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

#include <lely/util/spscring.h>

TEST_GROUP(Util_Spscring) {
  spscring ring;
  size_t ring_size = 15UL;

  TEST_SETUP() { spscring_init(&ring, ring_size); }
};

TEST(Util_Spscring, Spscring_Size) {
  CHECK_EQUAL(ring_size, spscring_size(&ring));
}

TEST(Util_Spscring, SpscringPCapacity_Empty) {
  CHECK_EQUAL(ring_size, spscring_p_capacity(&ring));
}

static void
spscring_p_fill(spscring* const ring, const size_t how_many) {
  size_t indices = how_many;

  spscring_p_alloc(ring, &indices);
  spscring_p_commit(ring, indices);
}

static void
spscring_c_fill(spscring* const ring, const size_t how_many) {
  size_t indices = how_many;

  spscring_c_alloc(ring, &indices);
  spscring_c_commit(ring, indices);
}

TEST(Util_Spscring, SpscringPCapacity_OneAdded) {
  spscring_p_fill(&ring, 1UL);

  CHECK_EQUAL(ring_size - 1UL, spscring_p_capacity(&ring));
}

TEST(Util_Spscring, SpscringPCapacity_ManyAdded) {
  spscring_p_fill(&ring, 2UL);

  CHECK_EQUAL(ring_size - 2UL, spscring_p_capacity(&ring));
}

TEST(Util_Spscring, SpscringPCapacity_MaxAdded) {
  spscring_p_fill(&ring, ring_size);

  CHECK_EQUAL(0UL, spscring_p_capacity(&ring));
}

TEST(Util_Spscring, SpscringPCapacity_MoreThanMaxAdded) {
  spscring_p_fill(&ring, ring_size + 1UL);

  CHECK_EQUAL(0UL, spscring_p_capacity(&ring));
}

TEST(Util_Spscring, SpscringPCapacityNoWrap_Empty) {
  CHECK_EQUAL(ring_size, spscring_p_capacity_no_wrap(&ring));
}

TEST(Util_Spscring, SpscringPCapacityNoWrap_OneAdded) {
  spscring_p_fill(&ring, 1UL);

  CHECK_EQUAL(ring_size - 1UL, spscring_p_capacity_no_wrap(&ring));
}

TEST(Util_Spscring, SpscringPCapacityNoWrap_ManyAdded) {
  spscring_p_fill(&ring, 2UL);

  CHECK_EQUAL(ring_size - 2UL, spscring_p_capacity_no_wrap(&ring));
}

TEST(Util_Spscring, SpscringPCapacityNoWrap_MaxAdded) {
  spscring_p_fill(&ring, ring_size);

  CHECK_EQUAL(0UL, spscring_p_capacity_no_wrap(&ring));
}

TEST(Util_Spscring, SpscringPCapacityNoWrap_MoreThanMaxAdded) {
  spscring_p_fill(&ring, ring_size + 1UL);

  CHECK_EQUAL(0UL, spscring_p_capacity_no_wrap(&ring));
}

TEST(Util_Spscring, SpscringPAlloc_InEmpty) {
  CHECK_EQUAL(0UL, spscring_p_alloc(&ring, &ring_size));
}

TEST(Util_Spscring, SpscringPAlloc_AllocateGreaterThanCapacity) {
  size_t great_size = ring_size + 1UL;

  CHECK_EQUAL(0UL, spscring_p_alloc(&ring, &great_size));
}

TEST(Util_Spscring, SpscringPAlloc_OneAdded) {
  spscring_p_fill(&ring, 1UL);

  CHECK_EQUAL(1UL, spscring_p_alloc(&ring, &ring_size));
}

TEST(Util_Spscring, SpscringPAlloc_ManyAdded) {
  spscring_p_fill(&ring, 2UL);

  CHECK_EQUAL(2UL, spscring_p_alloc(&ring, &ring_size));
}

TEST(Util_Spscring, SpscringPAllocNoWrap_AllocateZeroWhenEmpty) {
  size_t alloc_size = 0UL;

  CHECK_EQUAL(0UL, spscring_p_alloc_no_wrap(&ring, &alloc_size));
}

TEST(Util_Spscring, SpscringPAllocNoWrap_AllocateOneWhenEmpty) {
  size_t alloc_size = 1UL;

  CHECK_EQUAL(0UL, spscring_p_alloc_no_wrap(&ring, &alloc_size));
}

TEST(Util_Spscring, SpscringPAllocNoWrap_AllocateManyWhenEmpty) {
  size_t alloc_size = 2UL;

  CHECK_EQUAL(0UL, spscring_p_alloc_no_wrap(&ring, &alloc_size));
}

TEST(Util_Spscring, SpscringPAllocNoWrap_AllocateZeroWhenOneAdded) {
  spscring_p_fill(&ring, 1UL);
  size_t alloc_size = 0UL;

  CHECK_EQUAL(1UL, spscring_p_alloc_no_wrap(&ring, &alloc_size));
}

TEST(Util_Spscring, SpscringPAllocNoWrap_AllocateOneWhenOneAdded) {
  spscring_p_fill(&ring, 1UL);
  size_t alloc_size = 1UL;

  CHECK_EQUAL(1UL, spscring_p_alloc_no_wrap(&ring, &alloc_size));
}

TEST(Util_Spscring, SpscringPAllocNoWrap_AllocateManyWhenOneAdded) {
  spscring_p_fill(&ring, 1UL);
  size_t alloc_size = 2UL;

  CHECK_EQUAL(1UL, spscring_p_alloc_no_wrap(&ring, &alloc_size));
}

TEST(Util_Spscring, SpscringPAllocNoWrap_AllocateZeroWhenManyAdded) {
  spscring_p_fill(&ring, 2UL);
  size_t alloc_size = 0UL;

  CHECK_EQUAL(2UL, spscring_p_alloc_no_wrap(&ring, &alloc_size));
}

TEST(Util_Spscring, SpscringPAllocNoWrap_AllocateOneWhenManyAdded) {
  spscring_p_fill(&ring, 2UL);
  size_t alloc_size = 1UL;

  CHECK_EQUAL(2UL, spscring_p_alloc_no_wrap(&ring, &alloc_size));
}

TEST(Util_Spscring, SpscringPAllocNoWrap_AllocateManyWhenManyAdded) {
  spscring_p_fill(&ring, 2UL);
  size_t alloc_size = 2UL;

  CHECK_EQUAL(2UL, spscring_p_alloc_no_wrap(&ring, &alloc_size));
}

class ProducerConsumerSignaller {
 public:
  static bool consumer_signal_called;
  static bool producer_signal_called;

  ~ProducerConsumerSignaller() { reset_called_signal_flags(); }

  static void
  producer_signal(spscring*, void*) {
    producer_signal_called = true;
  }

  static void
  consumer_signal(spscring*, void*) {
    consumer_signal_called = true;
  }

  static void
  reset_called_signal_flags() {
    producer_signal_called = false;
    consumer_signal_called = false;
  }
};
bool ProducerConsumerSignaller::consumer_signal_called = false;
bool ProducerConsumerSignaller::producer_signal_called = false;

TEST(Util_Spscring, SpscringPCommit_InvokesConsumerSignal) {
  int argument = 0;
  ProducerConsumerSignaller pcs;
  spscring_c_submit_wait(&ring, ring_size, pcs.consumer_signal, &argument);
  spscring_p_alloc(&ring, &ring_size);

  spscring_p_commit(&ring, ring_size);

  CHECK_EQUAL(true, pcs.consumer_signal_called);
}

TEST(Util_Spscring,
     SpscringPSubmitWait_SigSubmittedIndsNotAvailDemandLessThanSize) {
  int argument = 0;
  ProducerConsumerSignaller pcs;

  spscring_p_fill(&ring, 1UL);

  CHECK_EQUAL(1UL, spscring_p_submit_wait(&ring, ring_size, pcs.producer_signal,
                                          &argument));
}

TEST(Util_Spscring,
     SpscringPSubmitWait_SigSubmittedIndsNotAvailDemandMoreThanSize) {
  int argument = 0;
  ProducerConsumerSignaller pcs;

  spscring_p_fill(&ring, 1UL);

  CHECK_EQUAL(1UL, spscring_p_submit_wait(&ring, ring_size + 1UL,
                                          pcs.producer_signal, &argument));
}

TEST(Util_Spscring, SpscringPSubmitWait_SigSubmittedIndicesAvailable) {
  int argument = 0;
  ProducerConsumerSignaller pcs;

  CHECK_EQUAL(0UL, spscring_p_submit_wait(&ring, ring_size, pcs.producer_signal,
                                          &argument));
}

TEST(Util_Spscring, SpscringPAbortWait_AbortsWait) {
  int argument = 0;
  ProducerConsumerSignaller pcs;

  spscring_p_fill(&ring, 1UL);
  spscring_p_submit_wait(&ring, ring_size, pcs.producer_signal, &argument);

  size_t consum_alloc = 1UL;
  spscring_c_alloc(&ring, &consum_alloc);
  spscring_p_abort_wait(&ring);
  spscring_c_commit(&ring, consum_alloc);

  CHECK_EQUAL(false, pcs.producer_signal_called);
}

TEST(Util_Spscring, SpscringCCapacity_Empty) {
  spscring_p_fill(&ring, 0UL);

  CHECK_EQUAL(0UL, spscring_c_capacity(&ring));
}

TEST(Util_Spscring, SpscringCCapacity_OneAdded) {
  spscring_p_fill(&ring, 1UL);

  CHECK_EQUAL(1UL, spscring_c_capacity(&ring));
}

TEST(Util_Spscring, SpscringCCapacity_ManyAdded) {
  spscring_p_fill(&ring, 2UL);

  CHECK_EQUAL(2UL, spscring_c_capacity(&ring));
}

TEST(Util_Spscring, SpscringCCapacity_MaxAdded) {
  spscring_p_fill(&ring, ring_size);

  CHECK_EQUAL(ring_size, spscring_c_capacity(&ring));
}

TEST(Util_Spscring, SpscringCCapacity_MoreThanMaxAdded) {
  spscring_p_fill(&ring, ring_size + 1UL);

  CHECK_EQUAL(ring_size, spscring_c_capacity(&ring));
}

TEST(Util_Spscring, SpscringCCapacityNoWrap_Empty) {
  spscring_p_fill(&ring, 0UL);

  CHECK_EQUAL(0UL, spscring_c_capacity_no_wrap(&ring));
}

TEST(Util_Spscring, SpscringCCapacityNoWrap_OneAdded) {
  spscring_p_fill(&ring, 1UL);

  CHECK_EQUAL(1UL, spscring_c_capacity_no_wrap(&ring));
}

TEST(Util_Spscring, SpscringCCapacityNoWrap_ManyAdded) {
  spscring_p_fill(&ring, 2UL);

  CHECK_EQUAL(2UL, spscring_c_capacity_no_wrap(&ring));
}

TEST(Util_Spscring, SpscringCCapacityNoWrap_MaxAdded) {
  spscring_p_fill(&ring, ring_size);

  CHECK_EQUAL(ring_size, spscring_c_capacity_no_wrap(&ring));
}

TEST(Util_Spscring, SpscringCCapacityNoWrap_MoreThanMaxAdded) {
  spscring_p_fill(&ring, ring_size + 1UL);

  CHECK_EQUAL(ring_size, spscring_c_capacity_no_wrap(&ring));
}

TEST(Util_Spscring, SpscringCAlloc_InEmpty) {
  CHECK_EQUAL(0UL, spscring_c_alloc(&ring, &ring_size));
}

TEST(Util_Spscring, SpscringCAlloc_AllocateGreaterThanCapacity) {
  size_t great_size = ring_size + 1UL;

  CHECK_EQUAL(0UL, spscring_c_alloc(&ring, &great_size));
}

TEST(Util_Spscring, SpscringCAlloc_OneAdded) {
  spscring_c_fill(&ring, 1UL);

  CHECK_EQUAL(0UL, spscring_c_alloc(&ring, &ring_size));
}

TEST(Util_Spscring, SpscringCAlloc_ManyAdded) {
  spscring_c_fill(&ring, 2UL);

  CHECK_EQUAL(0UL, spscring_c_alloc(&ring, &ring_size));
}

TEST(Util_Spscring, SpscringCAllocNoWrap_InEmpty) {
  CHECK_EQUAL(0UL, spscring_c_alloc_no_wrap(&ring, &ring_size));
}

TEST(Util_Spscring, SpscringCAllocNoWrap_AllocateGreaterThanCapacity) {
  size_t great_size = ring_size + 1UL;

  CHECK_EQUAL(0UL, spscring_c_alloc_no_wrap(&ring, &great_size));
}

TEST(Util_Spscring, SpscringCAllocNoWrap_OneAdded) {
  spscring_c_fill(&ring, 1UL);

  CHECK_EQUAL(0UL, spscring_c_alloc_no_wrap(&ring, &ring_size));
}

TEST(Util_Spscring, SpscringCAllocNoWrap_ManyAdded) {
  spscring_c_fill(&ring, 2UL);

  CHECK_EQUAL(0UL, spscring_c_alloc_no_wrap(&ring, &ring_size));
}

TEST(Util_Spscring, SpscringCCommit_EmptyZeroCommited) {
  CHECK_EQUAL(0UL, spscring_c_commit(&ring, 0UL));
}

TEST(Util_Spscring, SpscringCCommit_EmptyOneCommited) {
  size_t size_to_alloc = 1UL;

  spscring_c_alloc(&ring, &size_to_alloc);

  CHECK_EQUAL(0UL, spscring_c_commit(&ring, size_to_alloc));
}

TEST(Util_Spscring, SpscringCCommit_EmptyManyCommited) {
  size_t size_to_alloc = 2UL;

  spscring_c_alloc(&ring, &size_to_alloc);

  CHECK_EQUAL(0UL, spscring_c_commit(&ring, size_to_alloc));
}

TEST(Util_Spscring, SpscringCCommit_OneAddedZeroCommited) {
  spscring_p_fill(&ring, 1UL);

  CHECK_EQUAL(0UL, spscring_c_commit(&ring, 0UL));
}

TEST(Util_Spscring, SpscringCCommit_OneAddedOneCommited) {
  spscring_p_fill(&ring, 1UL);
  spscring_c_fill(&ring, 1UL);
  size_t size_to_alloc = 1UL;

  spscring_c_alloc(&ring, &size_to_alloc);

  CHECK_EQUAL(1UL, spscring_c_commit(&ring, size_to_alloc));
}

TEST(Util_Spscring, SpscringCCommit_ManyAddedZeroCommited) {
  spscring_p_fill(&ring, 2UL);
  spscring_c_fill(&ring, 1UL);
  size_t size_to_alloc = 0UL;

  spscring_c_alloc(&ring, &size_to_alloc);

  CHECK_EQUAL(1UL, spscring_c_commit(&ring, size_to_alloc));
}

TEST(Util_Spscring, SpscringCCommit_ManyAddedOneCommited) {
  spscring_p_fill(&ring, 2UL);
  spscring_c_fill(&ring, 2UL);
  size_t size_to_alloc = 1UL;

  spscring_c_alloc(&ring, &size_to_alloc);

  CHECK_EQUAL(2UL, spscring_c_commit(&ring, size_to_alloc));
}

TEST(Util_Spscring, SpscringCCommit_ManyAddedManyCommited) {
  spscring_p_fill(&ring, 2UL);
  spscring_c_fill(&ring, 2UL);
  size_t size_to_alloc = 2UL;

  spscring_c_alloc(&ring, &size_to_alloc);

  CHECK_EQUAL(2UL, spscring_c_commit(&ring, size_to_alloc));
}

TEST(Util_Spscring, SpscringCCommit_InvokesProducerSignal) {
  int argument = 0;
  spscring_p_fill(&ring, 1UL);

  ProducerConsumerSignaller pcs;
  spscring_p_submit_wait(&ring, ring_size, pcs.producer_signal, &argument);
  size_t consum_alloc = 1UL;
  spscring_c_alloc(&ring, &consum_alloc);

  spscring_c_commit(&ring, consum_alloc);
  CHECK_EQUAL(true, pcs.producer_signal_called);
}

TEST(Util_Spscring, SubmitWait_AvailableForReading) {
  spscring_p_fill(&ring, ring_size);
  int argument = 0;

  ProducerConsumerSignaller pcs;
  CHECK_EQUAL(0UL, spscring_c_submit_wait(&ring, ring_size, pcs.consumer_signal,
                                          &argument));
}

TEST(Util_Spscring, SpscringCSubmitWait_NotAvailableForReading) {
  int argument = 0;

  ProducerConsumerSignaller pcs;
  CHECK_EQUAL(1UL, spscring_c_submit_wait(&ring, ring_size, pcs.consumer_signal,
                                          &argument));
}

TEST(Util_Spscring, SpscringCSubmitWait_ConsumerSignalCalledWhenDataAvailable) {
  int argument = 0;
  ProducerConsumerSignaller pcs;
  spscring_c_submit_wait(&ring, ring_size, pcs.consumer_signal, &argument);

  spscring_p_alloc(&ring, &ring_size);
  spscring_p_commit(&ring, ring_size);

  CHECK_EQUAL(true, pcs.consumer_signal_called);
}

TEST(Util_Spscring,
     SpscringCSubmitWait_ConsumerSigCalledWhenDataAvailMoreThanMaxRequested) {
  int argument = 0;
  ProducerConsumerSignaller pcs;
  spscring_c_submit_wait(&ring, ring_size + 1, pcs.consumer_signal, &argument);

  spscring_p_alloc(&ring, &ring_size);
  spscring_p_commit(&ring, ring_size);

  CHECK_EQUAL(true, pcs.consumer_signal_called);
}

TEST(Util_Spscring, SpscringCAbortWait_ConsumerSigNotCalledWhenDataAvail) {
  int argument = 0;
  ProducerConsumerSignaller pcs;
  spscring_c_submit_wait(&ring, ring_size, pcs.consumer_signal, &argument);

  spscring_p_alloc(&ring, &ring_size);
  spscring_c_abort_wait(&ring);
  spscring_p_commit(&ring, ring_size);

  CHECK_EQUAL(false, pcs.consumer_signal_called);
}
