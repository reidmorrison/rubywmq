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
# Sample : Sample program to show how to write a server side application
#
#     This server times out after 60 seconds, but could be modified to
#     run forever. Example:
#         queue.each(:wait=>-1) do |message|
#
#     Note: - All calls are being performed under synchpoint control to
#             prevent messages being if the program terminates unexpectedly
#             or an error occurrs.
#             Uses:  :sync=>true
#           - Queue#each will backout any message changes if an excecption is raised 
#             but not handled within the each block
#           
#     A "well-behaved" WebSphere MQ application should adhere to the following rules:
#     - Perform puts and gets under synchpoint where applicable
#     - Only send replies to Request messages. No reply for Datagrams
#     - Set the message type to Reply when replying to a request message
#     - Reply with:
#         - Remaining Expiry (Ideally deduct any processing time since get)
#         - Same priority as received message
#         - Same persistence as received message
#     - Adhere to the Report options supplied for message and correlation id's 
#         in reply message
#     - All headers must be returned on reply messages
#         - This allows the calling application to store state information 
#           in these headers
#         - Unless of course if the relevant header is input only and used
#           for completing the request
#             - In this case any remaining headers should be returned
#               to the caller
#     - If an error occurs trying to process the message, an error message
#       must be sent back to the requesting application
#     - If the reply fails, it must be put to the dead letter queue
#       with the relevant dead letter header and reason
#     
#
require 'wmq'

WMQ::QueueManager.connect(:q_mgr_name=>'REID') do |qmgr|
  qmgr.open_queue(:q_name=>'TEST.QUEUE', :mode=>:input) do |queue|
    queue.each(:wait=>60000, :sync=>true) do |request|
      puts "Data Received: #{request.data}"
      begin
        reply = WMQ::Message.new
        reply.data = 'Echo back:'+request.data
        
        qmgr.put_to_reply_q(:message=>reply,           
                            :request_message=>request, # Only replies if message type is request
                            :sync=>true)
        
      rescue WMQ::WMQException => exc
        # Sending this message to the Dead Letter Queue
        #   Including a Dead Letter Header
        #   Could also include a rf_header with a description of the problem
        #     if :reason is insufficient
        p exc
        puts "Failed to reply to sender, try to put to dead letter queue"
        put_to_dead_letter_q(:message=>message, 
                             :reason=>WMQ::MQRC_UNKNOWN_REMOTE_Q_MGR,
                             :sync=>true)
      end
      qmgr.commit
    end
  end
  puts 'No more messages found after 60 second wait interval'
end
