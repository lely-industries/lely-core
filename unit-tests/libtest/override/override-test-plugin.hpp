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

#ifndef LELY_OVERRIDE_TEST_PLUGIN_HPP_
#define LELY_OVERRIDE_TEST_PLUGIN_HPP_

#include <stack>

#include <CppUTest/TestHarness.h>
#include <CppUTest/TestPlugin.h>

namespace Override {

/**
 * CppUTest plugin for override methods.
 * Maintains proper 'valid calls' values across tests.
 */
class OverridePlugin : public TestPlugin {
 public:
  OverridePlugin();
  ~OverridePlugin();

  static OverridePlugin* getCurrent();

  void postTestAction(UtestShell&, TestResult&) override;

  void setForNextTest(int& vc, int target_value);

 private:
  class CleanUp;
  std::stack<CleanUp> cleanups;
};

}  // namespace Override

#endif  // !LELY_OVERRIDE_TEST_PLUGIN_HPP_
