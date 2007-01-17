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

    # Put a reply message back to the sender
    #
    #   The :message is sent to the queue and queue manager specified in the
    #   :reply_to_q and :reply_to_q_mgr propoerties of the :request_message.
    #
    #   The following rules are followed before sending the reply:
    #   - Only send replies to Request messages. No reply for Datagrams
    #   - Set the message type to Reply when replying to a request message
    #   - Reply with:
    #     - Remaining Expiry (Ideally deduct any processing time since get)
    #     - Same priority as received message
    #     - Same persistence as received message
    #   - Adhere to the Report options supplied for message and correlation id's
    #       in reply message
    #   - All headers must be returned on reply messages
    #     - This allows the calling application to store state information
    #       in these headers
    #     - Unless of course if the relevant header is input only and used
    #       for completing the request
    #       - In this case any remaining headers should be returned
    #         to the caller
    #
    # Parameters:
    #   * :request_message The message originally received
    #   * All the other parameters are the same as QueueManager#put
    #
    def put_to_reply_q(parms)
      # Send replies only if message type is request
      if parms[:request_message].descriptor[:msg_type] == WMQ::MQMT_REQUEST
        request = parms.delete(:request_message)

        reply = parms[:message] ||= Message.new(:data=>parms[:data])
        reply.descriptor[:msg_type]   = WMQ::MQMT_REPLY
        reply.descriptor[:expiry]     = request.descriptor[:expiry]
        reply.descriptor[:priority]   = request.descriptor[:priority]
        reply.descriptor[:persistence]= request.descriptor[:persistence]
        reply.descriptor[:format] = request.descriptor[:format]

        # Set Correlation Id based on report options supplied
        if request.descriptor[:report] & WMQ::MQRO_PASS_CORREL_ID != 0
          reply.descriptor[:correl_id] = request.descriptor[:correl_id]
        else
          reply.descriptor[:correl_id] = request.descriptor[:msg_id]
        end

        # Set Message Id based on report options supplied
        if request.descriptor[:report] & WMQ::MQRO_PASS_MSG_ID != 0
          reply.descriptor[:msg_id] = request.descriptor[:msg_id]
        end

        parms[:q_name]    = request.descriptor[:reply_to_q]
        parms[:q_mgr_name]= request.descriptor[:reply_to_q_mgr]
        return put(parms)
      else
        return false
      end
    end

    # Put a message to the Dead Letter Queue
    #
    #   If an error occurs when processing a datagram message
    #   it is necessary to move the message to the dead letter queue.
    #   I.e. An error message cannot be sent back to the sender because
    #        the original message was not a request message.
    #          I.e. msg_type != WMQ::MQMT_REQUEST
    #
    #   All existing message data, message descriptor and message headers
    #   are retained.
    #
    def put_to_dead_letter_q(parms)
      message = parms[:message] ||= Message.new(:data=>parms[:data])
      dlh = {
        :header_type     =>:dead_letter_header,
        :reason          =>parms.delete(:reason),
        :dest_q_name     =>parms.delete(:q_name),
        :dest_q_mgr_name =>self.name}

      message.headers.unshift(dlh)
      parms[:q_name]='SYSTEM.DEAD.LETTER.QUEUE'  #TODO Should be obtained from QMGR config
      return self.put(parms)
    end

  end

  # Message contains the message descriptor (MQMD), data
  # and any headers.
  #
  # Note:
  # * The format in the descriptor applies only to the format of the data portion,
  #   not the format of any included headers
  # * The message format can ONLY be supplied in the descriptor.
  #   * I.e. It is the format of the data, Not the headers.
  #   * On the wire formats are determined automatically by the :header_type key for 
  #     each header
  #   * Other WebSphere MQ interfaces require that the formats be "daisy-chained"
  #     * I.e. The MQMD.Format is actually the format of the first header
  #     * Ruby WMQ removes this tedious requirement and performs this
  #       requirement automatically under the covers
  #   * The format of any header should not be supplied in the descriptor or any header
  #
  # Message has the following attributes:
  # * descriptor = {
  #                                                           # WebSphere MQ Equivalent
  #     :format             => WMQ::MQFMT_STRING,             # MQMD.Format - Format of data only
  #                            WMQ::MQFMT_NONE                #   Do not supply header formats here
  #     :original_length    => Number                         # MQMD.OriginalLength
  #     :priority           => 0 .. 9                         # MQMD.Priority
  #     :put_time           => String                         # MQMD.PutTime
  #     :msg_id             => String                         ...
  #     :expiry             => Number
  #     :persistence        => Number
  #     :reply_to_q         => String
  #     :correl_id          => String
  #     :feedback           => Number
  #     :offset             => Number
  #     :report             => Number
  #     :msg_flags          => Number
  #     :reply_to_q_mgr     => String
  #     :appl_identity_data => String
  #     :put_appl_name      => String
  #     :user_identifier    => String
  #     :msg_seq_number     => Number
  #     :appl_origin_data   => String
  #     :accounting_token   => String
  #     :backout_count      => Number
  #     :coded_char_set_id  => Number
  #     :put_appl_type      => Number
  #     :msg_type           => Number
  #     :group_id           => String
  #     :put_date           => String
  #     :encoding           => Number
  #   }
  # * data => String
  # * headers => Array of Hashes
  #   * Note: Do not supply the format of any header. Ruby WMQ does this for you.
  #   
  #    The following headers are supported:
  #   * Rules And Formatting Header (RFH)
  #       :header_type => :rf_header
  #       :....
  #   * Rules and Formatting V2 Header (RFH2)
  #       ....
  #   * Dead Letter Header
  #   * CICS Header
  #   * IMS Header
  #   * Transmission Queue Header
  #   * ...
  class Message
    attr_reader :data, :descriptor, :headers
    attr_writer :data, :descriptor, :headers
  end

end
