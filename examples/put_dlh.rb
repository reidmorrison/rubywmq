#
# Sample : put() : Put a message to a queue with a dead letter header
#          Open the queue so that multiple puts can be performed
#
require 'wmq'

WMQ::QueueManager.connect(q_mgr_name: 'REID') do |qmgr|
  qmgr.open_queue(q_name: 'TEST.QUEUE', mode: :output) do |queue|
    message         = WMQ::Message.new
    message.data    = 'Hello World'
    message.headers = [
      {
        header_type:     :dead_letter_header,
        reason:          WMQ::MQRC_UNKNOWN_REMOTE_Q_MGR,
        dest_q_name:     'ORIGINAL_QUEUE_NAME',
        dest_q_mgr_name: 'BAD_Q_MGR'
      }
    ]

    message.descriptor[:format] = WMQ::MQFMT_STRING

    queue.put(:message => message)
  end
end
