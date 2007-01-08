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
# Sample : get() : Retrieve a single message from a queue
#          If no messages are on the queue, message.data is nil
#
#          Calls MQGET
#
# Force Client connection in case we have a queue manager locally
require 'wmq/wmq_client'

WMQ::QueueManager.connect(
            :channel_name    => 'SYSTEM.DEF.SVRCONN',
            :transport_type  => WMQ::MQXPT_TCP,
            :connection_name => 'localhost(1414)' ) do |qmgr|
  qmgr.open_queue(:q_name=>'TEST.QUEUE', :mode=>:input) do |queue|

    message = WMQ::Message.new
    if queue.get(:message => message)
      puts "Headers Received:"
      message.headers.each {|header| p header}
      
      puts "Data Received: #{message.data}"
    else
      puts 'No message available'
    end
  end
end

