#
# Sample : get() : Retrieve a single message from a queue
#          If no messages are on the queue, message.data is nil
#
# The Client connection is determined by the :connection_name parameter supplied
# to QueueManager::connect or QueueManager::new
#
# If :connection_name is not present, a WebSphere MQ Server connection will be used
# I.e. Local server connection
#
require 'wmq'

WMQ::QueueManager.connect(
  connection_name: 'localhost(1414)',    # Use MQ Client Library
  channel_name:    'SYSTEM.DEF.SVRCONN', # Optional, since this is the default value
  transport_type:  WMQ::MQXPT_TCP        # Optional, since this is the default value
) do |qmgr|
  qmgr.open_queue(q_name: 'TEST.QUEUE', mode: :input) do |queue|

    message = WMQ::Message.new
    if queue.get(message: message)
      puts "Data Received: #{message.data}"

      puts 'Message Descriptor:'
      p message.descriptor

      puts 'Headers Received:'
      message.headers.each { |header| p header }
    else
      puts 'No message available'
    end
  end
end

