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
#          Create the queue if it does not already exist
#
#   Note : The queue name is that of the model queue
#          The dynamic_q_name is the actual queue name
#
#   This sample is more usefull if the model queue was a Permanent Dynamic one.
#   That way the queue would remain after termination of this code.
#   In this sample the queue will disappear when this program terminates
#
require 'wmq/wmq'

WMQ::QueueManager.connect(:q_mgr_name=>'REID') do |qmgr|
  qmgr.open_queue(:q_name => 'SYSTEM.DEFAULT.MODEL.QUEUE',
                  :dynamic_q_name => 'TEST.QUEUE.SAMPLE',
                  :mode  => :output
                  ) do |queue|
    queue.put(:data => 'Hello World')
  end
end
