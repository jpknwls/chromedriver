// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <list>
#include <string>

#include "base/json/json_reader.h"
#include "base/values.h"
#include "chrome/test/chromedriver/dom_tracker.h"
#include "chrome/test/chromedriver/status.h"
#include "chrome/test/chromedriver/stub_devtools_client.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class FakeDevToolsClient : public StubDevToolsClient {
 public:
  FakeDevToolsClient() {}
  virtual ~FakeDevToolsClient() {}

  std::string PopSentCommand() {
    std::string command;
    if (!sent_command_queue_.empty()) {
      command = sent_command_queue_.front();
      sent_command_queue_.pop_front();
    }
    return command;
  }

  // Overridden from DevToolsClient:
  virtual Status SendCommand(const std::string& method,
                             const base::DictionaryValue& params) OVERRIDE {
    sent_command_queue_.push_back(method);
    return Status(kOk);
  }
  virtual Status SendCommandAndGetResult(
      const std::string& method,
      const base::DictionaryValue& params,
      scoped_ptr<base::DictionaryValue>* result) OVERRIDE {
    return SendCommand(method, params);
  }

 private:
  std::list<std::string> sent_command_queue_;
};

}  // namespace

TEST(DomTracker, GetFrameIdForNode) {
  FakeDevToolsClient client;
  DomTracker tracker(&client);
  std::string frame_id;
  ASSERT_TRUE(tracker.GetFrameIdForNode(101, &frame_id).IsError());
  ASSERT_TRUE(frame_id.empty());

  const char nodes[] =
      "[{\"nodeId\":100,\"children\":"
      "    [{\"nodeId\":101},"
      "     {\"nodeId\":102,\"frameId\":\"f\"}]"
      "}]";
  base::DictionaryValue params;
  params.Set("nodes", base::JSONReader::Read(nodes));
  tracker.OnEvent("DOM.setChildNodes", params);
  ASSERT_TRUE(tracker.GetFrameIdForNode(101, &frame_id).IsError());
  ASSERT_TRUE(frame_id.empty());
  ASSERT_TRUE(tracker.GetFrameIdForNode(102, &frame_id).IsOk());
  ASSERT_STREQ("f", frame_id.c_str());

  tracker.OnEvent("DOM.documentUpdated", params);
  ASSERT_TRUE(tracker.GetFrameIdForNode(102, &frame_id).IsError());
  ASSERT_STREQ("DOM.getDocument", client.PopSentCommand().c_str());
}