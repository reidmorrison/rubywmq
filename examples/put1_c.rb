#
# Sample : put() : Put a single request message to a queue
#
require 'wmq'

WMQ::QueueManager.connect(q_mgr_name: 'REID') do |qmgr|
  message      = WMQ::Message.new
  message.data = 'Hello World'

  message.descriptor = {
    msg_type:   WMQ::MQMT_REQUEST,
    reply_to_q: 'TEST.REPLY.QUEUE'
  }

  qmgr.put(q_name: 'TEST.QUEUE', message: message)
end
