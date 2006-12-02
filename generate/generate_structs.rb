class GenerateStructs

  @@field_ignore_list = [ 'StrucId', 'Version' , 'StrucLength']

  def extract_struct (filename, struct_name)
    properties_list = []
    @struct_name = struct_name
    line_number = 0
    group_name = nil
    found = false
    File.open(filename) do |file|
      file.each do |line|
        line_number += 1
        line.rstrip!
        # Skip empty and comment lines
        if line.length > 0 && line !~ /^\s*\/\*/
          if !found
            found = true if line =~ /^\s*struct\s*tag#{struct_name}/
          else
            return(properties_list) if line =~ /^\s*\};/
            #puts line
            match = /\s*(MQ\w+)\s+(\w+);/.match(line)
            if match
              type = match[1]
              element = match[2]
              properties_list.push([type, element]) unless @@field_ignore_list.include? element
            end
          end
        end
      end
    end
    properties_list
  end

  def rubyize_name(name)
    name.gsub(/::/, '/').
         gsub(/([A-Z]+)([A-Z][a-z])/,'\1_\2').
         gsub(/([a-z\d])([A-Z])/,'\1_\2').
         tr("-", "_").
         downcase
  end

  def self.test_rubyize_name
    test = self.new
    [['a', 'a'],
     ['A', 'a'],
     ['Aa', 'aa'],
     ['AA', 'aa'],
     ['AaA', 'aa_a'],
     ['MyFieldName', 'my_field_name'],
     ['MMyFieldNName', 'm_my_field_n_name'],
     ['ABCdefGHIjKlMnOPQrSTuvwxYz', 'ab_cdef_gh_ij_kl_mn_op_qr_s_tuvwx_yz']
    ].each do |item|
      str = test.rubyize_name(item[0])
      raise("rubyize_name('#{item[0]}') == #{str} != '#{item[1]})") if str != item[1]
    end
  end

  def struct(properties_list, direction)
    variable = 'p'+@struct_name.downcase
    str = <<END_OF_STRING
void #{direction.downcase}_#{@struct_name.downcase}(VALUE hash, #{@struct_name}* #{variable})
{
    VALUE str;
    size_t size;
    size_t length;
END_OF_STRING

    if direction == 'TO'
      str << "    VALUE val;\n"
    else
      str << <<END_OF_STRING
    size_t i;
    char* pChar;
END_OF_STRING
    end
    str << <<END_OF_STRING

    Check_Type(hash, T_HASH);

    /* Process #{@struct_name} */
END_OF_STRING

    properties_list.each do |item|
      type = item[0]
      name = item[1]
      match = /(MQ\D+)/.match(type)
      type = "#{match[1]}S" if match[1] != type
      if direction == 'TO'
        if type == 'MQMDS'
          str << "    %-16s(hash, &#{variable}->%s);\n" % ['to_mqmd1', name]
        else
          str << "    %-16s(hash, %-30s #{variable}->%s)\n" % ["WMQ_HASH2#{type}", rubyize_name(name)+',',name]
        end
      else
        if type == 'MQMDS'
          str << "    %-16s(hash, &#{variable}->%s);\n" % ['from_mqmd1', name]
        else
          str << "    %-16s(hash, %-30s #{variable}->%s)\n" % ["WMQ_#{type}2HASH", rubyize_name(name)+',',name]
        end
      end
    end
    str <<  "}\n\n"
  end

  def wmq_struct(path, id_list, header_file, struct)
    str = ''
    properties = extract_struct(path+'/'+header_file, struct)
    properties.each do |item|
      id_list[rubyize_name(item[1])] = nil
    end

    str << struct(properties, 'TO')
    str << struct(properties, 'FROM')
  end

  def wmq_deblock(structs)
    str = <<END_OF_STRING
void Message_deblock(VALUE self, PMQMD pmqmd, PMQBYTE p_buffer, MQLONG total_length)
{
    PMQCHAR p_format   = pmqmd->Format;               /* Start with format in MQMD     */
    PMQBYTE p_data     = p_buffer;                    /* Pointer to start of data      */
    MQLONG  data_length= total_length;                /* length of data portion        */
    VALUE   headers    = rb_ary_new();
    VALUE   descriptor = rb_hash_new();
    MQLONG  size       = 0;

    from_mqmd(descriptor, pmqmd);
    rb_funcall(self, ID_descriptor_set, 1, descriptor);

    while (p_format)
    {
END_OF_STRING
    structs.each do |struct|
      if struct[:header]
        str << <<END_OF_STRING
        /* #{struct[:struct]}: #{struct[:header]} */
        if(strncmp (p_format, MQFMT_#{struct[:header].upcase}, sizeof(MQ_FORMAT_LENGTH)) == 0)
        {
            VALUE hash = rb_hash_new();
            P#{struct[:struct]} p_header = (P#{struct[:struct]})p_data;
            if(memcmp (p_header->StrucId, #{struct[:struct].upcase}_STRUC_ID, sizeof(p_header->StrucId)) != 0)
            {
                p_format = 0;    /* Bad Message received, do not deblock headers */
            }
            else
            {
                from_#{struct[:struct].downcase}(hash, p_header);
                rb_hash_aset(hash, ID2SYM(ID_header_type), ID2SYM(ID_#{struct[:header]}));
                rb_ary_push(headers, hash);

                p_format    = #{if struct[:no_format] then "0" else "p_header->Format" end};
#ifdef #{struct[:struct]}_CURRENT_LENGTH
                size        = p_header->StrucLength;
#else
                size        = sizeof(#{struct[:struct]});
#endif
                p_data      += size;
                data_length -= size;
            }
        }
        else
END_OF_STRING
      end
    end
# #{if struct[:no_format] then "0" else "strncmp (((P"+struct[:struct]+")p_data)->Format, MQFMT_"+struct[:header].upcase+", sizeof(MQ_FORMAT_LENGTH)) != 0" end}    
    str << <<END_OF_STRING
        {
            p_format = 0;
        }
    }

    rb_funcall(self, ID_headers_set, 1, headers);
    rb_funcall(self, ID_data_set, 1, rb_str_new(p_data, data_length));
}

END_OF_STRING
  end


  def wmq_build(structs)
    str = <<END_OF_STRING
int Message_build_header (VALUE hash, struct Message_build_header_arg* parg)
{
    VALUE   val = rb_hash_aref(hash, ID2SYM(ID_header_type));
    PMQBYTE p_data = *(parg->pp_buffer) + *(parg->p_data_offset);
    MQLONG  needed_size;

    if (!NIL_P(val) && (TYPE(val) == T_SYMBOL))
    {
        ID header_id = rb_to_id(val);
END_OF_STRING
    structs.each do |struct|
      if struct[:header]
  str << <<END_OF_STRING
        if (header_id == ID_#{struct[:header]})
        {
            static #{struct[:struct]} #{struct[:struct]}_DEF = {#{struct[:struct]}_DEFAULT};
            #{struct[:defaults]}

            if(parg->trace_level>2)
                printf ("WMQ::Queue#put Found #{struct[:header]}\\n");

            needed_size = *(parg->p_data_offset) + sizeof(#{struct[:struct]}) + parg->data_length;

            /* Is buffer large enough for headers */
            if(needed_size >= *(parg->p_buffer_size))
            {
                PMQBYTE old_buffer = *(parg->pp_buffer);
                needed_size += 512;                   /* Additional space for subsequent headers */

                if(parg->trace_level>2)
                    printf ("WMQ::Queue#put Reallocating buffer from %ld to %ld\\n", *(parg->p_buffer_size), needed_size);

                *(parg->p_buffer_size) = needed_size;
                *(parg->pp_buffer) = ALLOC_N(char, needed_size);
                memcpy(*(parg->pp_buffer), old_buffer, *(parg->p_data_offset));
                free(old_buffer);
            }

            memcpy(p_data, &#{struct[:struct]}_DEF, sizeof(#{struct[:struct]}));
            to_#{struct[:struct].downcase}(hash, (P#{struct[:struct]})p_data);

            *(parg->p_data_offset) += sizeof(#{struct[:struct]});

            if(parg->trace_level>2)
                printf ("WMQ::Queue#put data offset:%ld\\n", *(parg->p_data_offset));

        }
        else
END_OF_STRING
      end
    end
  str << <<END_OF_STRING
        {
            rb_raise(rb_eArgError, "Unknown :header_type supplied in WMQ::Message#headers array");
        }
    }
    else
    {
        rb_raise(rb_eArgError, "Mandatory parameter :header_type missing from header entry in WMQ::Message#headers array");
    }

    return 0;                                         /* Continue */
}

END_OF_STRING
  end

  def wmq_struct_ids(id_list)
    str = ''
    key_list = id_list.keys.sort
    key_list.each do |key, value|
      str << "static ID ID_#{key.sub('=', '_set')};\n"
    end

    str << <<END_OF_STRING

void wmq_structs_id_init()
{
END_OF_STRING
    key_list.each do |key, value|
      str << "    ID_%-20s = rb_intern(\"%s\");\n"% [key.sub('=', '_set'), key]
    end
    str << "}\n\n"
  end

  def wmq_structs(path)
    str = <<END_OF_STRING
/* --------------------------------------------------------------------------
 *  Copyright 2006 J. Reid Morrison. Dimension Solutions, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 * --------------------------------------------------------------------------
 *
 *  WARNING: DO NOT MODIFY THIS FILE
 *
 *  This file was generated by generate_structs.rb.
 *
 * --------------------------------------------------------------------------*/

#include "wmq.h"

END_OF_STRING
    id_list = {
          'descriptor='=>nil,
          'headers='=>nil,
          'data='=>nil,
          'header_type'=>nil,
    }
    structs = [
      # Message Descriptor
      {:file=>'cmqc.h', :struct=>'MQMD'},

      # Message Descriptor 1
      {:file=>'cmqc.h', :struct=>'MQMD1'},

      # Dead Letter Header
      {:file=>'cmqc.h', :struct=>'MQDLH', :header=>'dead_letter_header',
       :defaults=>'MQDLH_DEF.CodedCharSetId = MQCCSI_INHERIT;'},

      # CICS bridge header
      {:file=>'cmqc.h', :struct=>'MQCIH', :header=>'cics'},

      # Distribution header
      {:file=>'cmqc.h', :struct=>'MQDH',  :header=>'dist_header'},

      # IMS information header
      {:file=>'cmqc.h', :struct=>'MQIIH', :header=>'ims'},

      # Rules and formatting header
      {:file=>'cmqc.h', :struct=>'MQRFH'},

      # Rules and formatting header2
      {:file=>'cmqc.h', :struct=>'MQRFH2'},

      # Reference message header (send large files over channels)
      {:file=>'cmqc.h', :struct=>'MQRMH', :header=>'ref_msg_header'},

      # Trigger Message
      {:file=>'cmqc.h', :struct=>'MQTM', :header=>'trigger', :no_format=>true},

      # Trigger Message 2 (character format)
      {:file=>'cmqc.h', :struct=>'MQTMC2'},

      # Work Information header
      {:file=>'cmqc.h', :struct=>'MQWIH', :header=>'work_info_header'},

      # Transmission-queue header - Todo: Need to deal with MQMDE
      {:file=>'cmqc.h', :struct=>'MQXQH', :header=>'xmit_q_header', :no_format=>true},
    ]
    structs.each {|struct| id_list[struct[:header]]=nil if struct[:header]}
    str_methods = ''

    structs.each { |struct| str_methods << wmq_struct(path, id_list, struct[:file], struct[:struct]) }
    str << wmq_struct_ids(id_list)
    str << str_methods
    str << wmq_deblock(structs)
    str << wmq_build(structs)
  end
  
  def self.generate(path)
    File.open('wmq_structs.c', 'w') {|file| file.write(GenerateStructs.new.wmq_structs(path))}
    puts 'Generated wmq_structs.c'
  end
end

if $0 == __FILE__
  path = ARGV[0] || raise("Mandatory parameter: 'WebSphere MQ Include path' is missing")
  path = path + '/'
  GenerateStructs.generate(path)
end
