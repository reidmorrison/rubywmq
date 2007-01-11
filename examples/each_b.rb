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
# Sample : each() : Retrieve all messages from a queue that
#          have the same correlation id
#
require 'wmq'

WMQ::QueueManager.connect(:q_mgr_name=>'REID') do |qmgr|
  qmgr.open_queue(:q_name=>'TEST.QUEUE', :mode=>:input) do |queue|
    message = WMQ::Message.new

    # Retrieve First message
    if queue.get(:message=>message)
      puts "Data Received: #{message.data}"

      #Retrieve all subsequent messages that have the same correlation id
      queue.each(:message=>message, :match=>WMQ::MQMO_MATCH_CORREL_ID) do |msg|
        puts "Matching Data Received: #{msg.data}"
      end
    else
      puts 'No message available'
    end
  end
  puts 'Completed.'
end
