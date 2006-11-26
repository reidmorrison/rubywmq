################################################################################
#  Copyright 2006 J. Reid Morrison. Dimension Solutions, Inc.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
################################################################################

#
# Sample : put() : Put a single message to a queue
#          Open the queue so that multiple puts can be performed
#          Ensure that all messages have the same correlation id
#          Calls MQPUT
#
require 'wmq'

WMQ::QueueManager.connect(:q_mgr_name=>'REID') do |qmgr|
  qmgr.open_queue(:q_name=>'TEST.QUEUE', :mode=>:output) do |queue|

    message = WMQ::Message.new

    # First message
    #   Results in a WMQ generated msg_id and empty correl_id
    message.data = 'Hello World'
    queue.put(:message => message)

    # Second message
    #   new_msg_id will cause the second message to have a new message id
    #   otherwise both messages will have the same message and correlation id's
    #   This message will have the same correlation id as the first message (empty)
    message.data = 'Hello Again'
    p message.descriptor
    queue.put(:message => message, :new_msg_id => true)
  end
end
