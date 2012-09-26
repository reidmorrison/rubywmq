require 'mkmf'
generate_sources_path = File.join(File.dirname(__FILE__), 'generate')
$LOAD_PATH.unshift generate_sources_path
require 'generate_reason'
require 'generate_const'
require 'generate_structs'

include_path = ''
if RUBY_PLATFORM =~ /win|mingw/i
  include_path = 'C:\Program Files\IBM\WebSphere MQ\tools\c\include'
  dir_config('mqm', include_path, '.')
else
  include_path = '/opt/mqm/inc'
  #dir_config('mqm', include_path, '/opt/mqm/lib')
end

have_header('cmqc.h')

# Check for WebSphere MQ Server library
unless (RUBY_PLATFORM =~ /win/i) || (RUBY_PLATFORM =~ /solaris/i) || (RUBY_PLATFORM =~ /linux/i)
  have_library('mqm')
end

# Generate Source Files
GenerateReason.generate(include_path+'/')
GenerateConst.generate(include_path+'/', File.dirname(__FILE__) + '/../lib/wmq')
GenerateStructs.new(include_path+'/', generate_sources_path).generate

# Generate Makefile
create_makefile('wmq/wmq')
