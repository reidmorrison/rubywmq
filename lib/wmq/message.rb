# Message Ruby methods
module WMQ
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
    attr_accessor :data, :descriptor, :headers
  end

end
