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
    
    # Warning: Do not use, this will go away
    #
    # Add a dead letter header
    # 
    # Moves the following fields from Options (MQMD) to the Dead Letter header:
    # 'put_appl_type', 'put_appl_name', 'put_date', 'put_time',
    # 'encoding', 'coded_char_set_id', 'format'
    # 
    # Sets format of MQMD to MQDEAD 
    # 
    def add_dead_letter_info(dlh)
      [:put_appl_type, :put_appl_name, :put_date, :put_time,
       :encoding, :coded_char_set_id, :format ].each do |item|
        value = @descriptor.delete(item)
        dlh[item] = value if value
      end
      
      @descriptor[:format] = 'MQDEAD'
      @headers[:dead_letter_header] = dlh
      dlh
    end
    
    # Warning: Do not use, this will go away
    # 
    # Extract DLH values that actually belong to DLH from MQMD
    # Leaving MQMD with values that belong to the data
    # Returns the DLH
    def extract_dead_letter_info
      dlh = @headers.delete(:dead_letter_header)
      return nil unless dlh
      
      # Either Channel Receiver, or MQMon file export/import is messing up these fields    
      #'encoding', 'coded_char_set_id',    
      [:put_appl_type, :put_appl_name, :put_date, :put_time,
      :format ].each do |item|
        dl_value = @descriptor[item]
        @descriptor[item] = dlh[item]
        dlh[item] = dl_value
      end
      
      dlh.delete(:format)
      dlh
    end
  end
  
end
