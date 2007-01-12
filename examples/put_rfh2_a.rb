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
# Sample : put() : Put a message to a queue with a Refernce header
#          Open the queue so that multiple puts can be performed
#
require 'wmq'

# The Rules Format header2 (MQRFH2) allows a an XML-like string to be passed as a header
# to the data.
#   

WMQ::QueueManager.connect(:q_mgr_name=>'REID') do |qmgr|
  qmgr.open_queue(:q_name=>'TEST.QUEUE', :mode=>:output) do |queue|
    message = WMQ::Message.new
    message.data = 'Hello World'
    
    message.headers = [
     {:header_type =>:rf_header_2,
      :xml => '<hello>to the world</hello>'
     }]
    
    message.descriptor[:format] = WMQ::MQFMT_STRING
    
    queue.put(:message=>message)
  end
end

