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
# Sample : request.rb
# 
#          Sample program that demonstrates how to send a request message
#          and then block until the response is received from the server.
#          
#          A temporary Dynamic Reply To Queue is used with non-persistent messaging
#
require 'rubygems'
require 'wmq'

wait_seconds = 30

WMQ::QueueManager.connect(:q_mgr_name=>'REID') do |qmgr|
  qmgr.open_queue(:q_name        => 'SYSTEM.DEFAULT.MODEL.QUEUE',
                  :dynamic_q_name=> 'REQUEST.*',
                  :mode          => :input
                 ) do |reply_queue|
                 
    message = WMQ::Message.new()
    message.descriptor = { :msg_type      => WMQ::MQMT_REQUEST,
                           :reply_to_q    => reply_queue.name,
                           :reply_to_q_mgr=> qmgr.name,
                           :format        => WMQ::MQFMT_STRING,
                           :expiry        => wait_seconds*10}  # Measured in tenths of a second
    message.data = 'Hello World'        
                  
    # Send request message
    qmgr.put(:q_name=>'TEST.QUEUE', :message=>message)
    
    # Copy outgoing Message id to correlation id
    message.descriptor[:correl_id]=message.descriptor[:msg_id]

    # Wait for reply 
    #   Only get the message that matches the correlation id set above
    if reply_queue.get(:wait=>wait_seconds*1000, :message=>message, :match=>WMQ::MQMO_MATCH_CORREL_ID)
      puts "Received:"
      puts message.data
    else
      # get returns false when no message received. Also: message.data = nil
      puts "Timed Out waiting for a reply from the server"
    end
  end
end
