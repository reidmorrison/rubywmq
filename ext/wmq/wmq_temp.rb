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

module WMQ

  # Temporary placeholder until the following code is moved to 'C'
   
  #
  class QueueManager
    def method_missing(name, *args)
      if args.size == 1
        self.execute({:command=>name}.merge(args[0]))
      elsif args.size == 0
        self.execute({:command=>name})
      else
        raise("Invalid arguments supplied to QueueManager#:#{name}, args:#{args}")
      end
    end
    
    # Execute any MQSC command against the queue manager
    # 
    # Example
    #   require 'wmq/wmq'
    #   require 'wmq/wmq_const_admin'
    #   WMQ::QueueManager.connect(:q_mgr_name=>'REID', :connection_name=>'localhost(1414)') do |qmgr|
    #     qmgr.mqsc('dis ql(*)').each {|item| p item }  
    #   end
    def mqsc(mqsc_text)
      self.execute(:command=>:escape, :escape_type=>WMQ::MQET_MQSC, :escape_text=>mqsc_text).collect {|item| item[:escape_text] }
    end
  end

  class Message
    attr_reader :data, :descriptor, :headers
    attr_writer :data, :descriptor, :headers
    
  end
  
end
