#
# Sample : put() : Put a single message to a queue
#          Create the queue if it does not already exist
#
#   Note : The queue name is that of the model queue
#          The dynamic_q_name is the actual queue name
#
#   This sample is more usefull if the model queue was a Permanent Dynamic one.
#   That way the queue would remain after termination of this code.
#   In this sample the queue will disappear when this program terminates
#
require 'wmq'

WMQ::QueueManager.connect(q_mgr_name: 'REID') do |qmgr|
  qmgr.open_queue(
    q_name:         'SYSTEM.DEFAULT.MODEL.QUEUE',
    dynamic_q_name: 'TEST.QUEUE.SAMPLE',
    mode:           :output
  ) do |queue|
    queue.put(data: 'Hello World')
  end
end
