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
# Sample : put() : Put a message to a queue with a Transmission header
#
require 'wmq'

WMQ::QueueManager.connect(:q_mgr_name=>'REID') do |qmgr|
  qmgr.open_queue(:q_name=>'TEST.QUEUE', :mode=>:output) do |queue|
    message = WMQ::Message.new
    message.data = "Test message from 'LOCALQMS1'"
    message.descriptor = {:original_length=>-1, :priority=>0, :put_time=>"18510170", :msg_id=>"AMQ LOCALQMS1   E\233\001\237 \000\003\005", :expiry=>-1, :persistence=>0, :reply_to_q=>"MQMON", :correl_id=>"AMQ LOCALQMS1   E\233\001\237 \000\003\004", :feedback=>0, :offset=>0, :report=>0, :msg_flags=>0, :reply_to_q_mgr=>"LOCALQMS1", :appl_identity_data=>"", :put_appl_name=>"LOCALQMS1", :user_identifier=>"mqm", :msg_seq_number=>1, :appl_origin_data=>"", :accounting_token=>"\026\001\005\025\000\000\000\271U\305\002\261\022\362\321\021D\3206\357\003\000\000\000\000\000\000\000\000\000\000\v", :backout_count=>0, :coded_char_set_id=>437, :put_appl_type=>7, :msg_type=>8, :group_id=>"", :put_date=>"20070109", :format=>"MQXMIT", :encoding=>546}
    message.headers = [{:priority=>0, :remote_q_mgr_name=>"OTHER.QMGR", :put_time=>"18510170", :msg_id=>"AMQ LOCALQMS1   E\233\001\237 \000\003\004", :expiry=>-1, :persistence=>0, :remote_q_name=>"OTHER.Q", :header_type=>:xmit_q_header, :reply_to_q=>"MQMON", :correl_id=>"", :feedback=>0, :report=>0, :reply_to_q_mgr=>"LOCALQMS1", :appl_identity_data=>"", :put_appl_name=>"uments\\MQ\\MQMon\\mqmonntp.exe", :user_identifier=>"mqm", :appl_origin_data=>"", :accounting_token=>"\026\001\005\025\000\000\000\271U\305\002\261\022\362\321\021D\3206\357\003\000\000\000\000\000\000\000\000\000\000\v", :backout_count=>0, :coded_char_set_id=>437, :put_appl_type=>11, :msg_type=>8, :put_date=>"20070109", :format=>"MQSTR", :encoding=>546}]
    
    queue.put(:message=>message)
  end
end

