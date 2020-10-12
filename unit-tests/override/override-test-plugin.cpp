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

#include "override-test-plugin.hpp"

#include <cassert>

#include <CppUTest/TestRegistry.h>

using Override::OverridePlugin;

static const char PLUGIN_NAME[] = "LelyOverridePlugin";

OverridePlugin::OverridePlugin() : TestPlugin(PLUGIN_NAME) {}

OverridePlugin::~OverridePlugin() {}

class OverridePlugin::CleanUp {
 public:
  explicit CleanUp(int& vc) : vc_(vc), org_value_(vc) {}
  ~CleanUp() { vc_ = org_value_; }

 private:
  int& vc_;
  const int org_value_;
};

void
OverridePlugin::postTestAction(UtestShell&, TestResult&) {
  while (!cleanups.empty()) cleanups.pop();
}

void
OverridePlugin::setForNextTest(int& vc, int target_value) {
  cleanups.emplace(vc);
  vc = target_value;
}

OverridePlugin*
OverridePlugin::getCurrent() {
  auto registry = TestRegistry::getCurrentRegistry();
  assert(registry != nullptr);
  auto plugin = registry->getPluginByName(PLUGIN_NAME);
  assert(plugin != nullptr);
  return static_cast<OverridePlugin*>(plugin);
}
