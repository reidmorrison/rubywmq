#
# Sample : put() : Put multiple Messages to a queue under the same group Id
#          Open the queue so that multiple puts can be performed
#          Ensure that all messages have the same group id
#          Supply our own user-defined group Id.
#          We also have to supply the message sequence number which starts at 1
#
require 'wmq'

# Put 5 messages in a single group onto the queue
total = 5

WMQ::QueueManager.connect(q_mgr_name: 'REID') do |qmgr|
  qmgr.open_queue(q_name: 'TEST.QUEUE', mode: :output) do |queue|
    message                       = WMQ::Message.new
    # Supply a unique Group Id, truncated to the MQ maximum for Group Id.
    message.descriptor[:group_id] = 'MyUniqueGroupId'

    total.times do |count|
      #   new_id => true causes subsequent messages to have unique message and
      #   correlation id's. Otherwise all messages will have the same message and
      #   correlation id's since the same message object is being
      #   re-used for all put calls
      message.data                        = "Hello:#{count}"

      # Set the message flag to indicate message is in a group
      # On the last message, set the last message flag
      message.descriptor[:msg_flags]      = (count < total-1) ? WMQ::MQMF_MSG_IN_GROUP : WMQ::MQMF_LAST_MSG_IN_GROUP
      message.descriptor[:msg_seq_number] = count + 1

      # By setting the put :options => WMQ::MQPMO_LOGICAL_ORDER then MQ will supply a unique Group Id
      # and MQ will automatically set the message sequence number for us.
      queue.put(message: message, new_id: true)
      p message.descriptor
    end
  end
end
