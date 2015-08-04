#
# Sample : Sample program to show how to write a server side application
#
#     This server times out after 60 seconds, but could be modified to
#     run forever. Example:
#         queue.each(wait: -1) do |message|
#
#     Note: - All calls are being performed under synchpoint control to
#             prevent messages being if the program terminates unexpectedly
#             or an error occurrs.
#             Uses:  sync: true
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
#     Note: - It is not recommended to run server side MQ applications over a client
#             connection.
#           - Client connections require substantially more error handling.
#             - E.g. Connection to queue manager can be lost due to netowk issues
#               - Need to go into some sort of retry state attempting
#                 to reconnect to the queue manager
#               - What about any work that was in progress?
#               - Need to re-open any queues
#               - Do any changes to other resources need to be undone first?
#                 - E.g. Database, File etc..
#             - etc....
#
require 'wmq'

WMQ::QueueManager.connect(q_mgr_name: 'REID') do |qmgr|
  qmgr.open_queue(q_name: 'TEST.QUEUE', mode: :input) do |queue|
    queue.each(wait: 60000, sync: true, convert: true) do |request|
      puts 'Data Received:'
      puts request.data

      begin
        reply      = WMQ::Message.new
        reply.data = 'Echo back:'+request.data

        qmgr.put_to_reply_q(
          message:         reply,
          request_message: request, # Only replies if message type is request
          sync:            true
        )

      rescue WMQ::WMQException => exc
        # Failed to send reply, put message to the Dead Letter Queue and add a dead letter header
        p exc
        puts 'Failed to reply to sender, Put to dead letter queue'
        qmgr.put_to_dead_letter_q(
          message: request,
          reason:  WMQ::MQRC_UNKNOWN_REMOTE_Q_MGR,
          q_name:  queue.name,
          sync:    true
        )
        # If it fails to put to the dead letter queue, this program will terminate and
        # the changes will be "backed out". E.g. Queue Full, ...
      end
      qmgr.commit
    end
  end
  puts 'No more messages found after 60 second wait interval'
end
