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
# Sample : put() : Put a single request message to a queue
#
require 'rubygems'
require 'wmq/wmq'

WMQ::QueueManager.connect(:q_mgr_name=>'REID') do |qmgr|
  message = WMQ::Message.new
  message.data = 'Hello World'
  
  message.descriptor = {
    :msg_type=> WMQ::MQMT_REQUEST,
    :reply_to_q=>'TEST.REPLY.QUEUE'}
  
  qmgr.put(:q_name=>'TEST.QUEUE', :message => message)
end
