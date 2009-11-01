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
require 'erb'

class GenerateStructs
  
  @@field_ignore_list = [ 'StrucId', 'Version' , 'StrucLength']
  
  # Store path to WebSphere MQ Structures
  def initialize(wmq_includepath, templates_path='.')
    @path = wmq_includepath
    @templates_path = templates_path
  end
  
  def extract_struct (filename, struct_name)
    properties_list = []
    line_number = 0
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
            match = /\s*(MQ\w+)\s+(\w+);/.match(line)
            if match
              type = match[1]
              element = match[2]
              properties_list.push([type, element])
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
  
  def generate_structs(erb)
    erb.result(binding)
  end
  
  def generate(target_filename = 'wmq_structs.c')
    erb = nil
    File.open(@templates_path+'/wmq_structs.erb') { |file| erb = ERB.new(file.read) }
    File.open(target_filename, 'w') {|file| file.write(generate_structs(erb)) }
    puts "Generated #{target_filename}"
  end
end

# TODO Auto Daisy-Chain Headers: Format, CodedCharSetId, Encoding
# TODO During Deblock validate that data size left at least matches struct size
if $0 == __FILE__
  path = ARGV[0] || raise("Mandatory parameter: 'WebSphere MQ Include path' is missing")
  path = path + '/'
  GenerateStructs.new(path, 'generate').generate
end
