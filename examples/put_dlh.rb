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
# Sample : put() : Put a message to a queue with a dead letter header
#          Open the queue so that multiple puts can be performed
#
#          Calls MQPUT
#
require 'wmq/wmq_client'

WMQ::QueueManager.connect(:q_mgr_name=>'REID', :connection_name=>'localhost(1414)', :trace_level=>4) do |qmgr|
  qmgr.open_queue(:q_name=>'TEST.QUEUE', :mode=>:output) do |queue|
    message = WMQ::Message.new
    message.data = 'Hello World'
    
    message.headers = [
     {:header_type =>:dead_letter_header,
      :reason      => WMQ::MQRC_UNKNOWN_REMOTE_Q_MGR,
      :dest_q_name =>'ORIGINAL_QUEUE_NAME',
      :dest_q_mgr_name =>'BAD_Q_MGR'}
    ]
    
    message.descriptor[:format] = WMQ::MQFMT_DEAD_LETTER_HEADER
    
    queue.put(:message=>message)
  end
end

