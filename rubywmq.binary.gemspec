require 'fileutils'
require 'rubygems'

if $0 == __FILE__
  FileUtils.mkdir_p('lib/wmq')
  Dir['ext/lib/*.rb'].each{|file| FileUtils.copy(file, File.join('lib/wmq', File.basename(file)))}
  FileUtils.copy('ext/wmq.so', 'lib/wmq/wmq.so')
end

spec = Gem::Specification.new do |s|
  s.name = 'rubywmq'
  s.version = '1.1.0'
  s.author = 'Reid Morrison'
  s.email = 'rubywmq@gmail.com'
  s.homepage = 'http://www.reidmorrison.com/rubywmq'
  s.platform = Gem::Platform::CURRENT
  s.summary = 'Ruby interface into WebSphere MQ (MQSeries)'
  s.description = 'RubyWMQ is a Ruby interface into WebSphere MQ, previously called MQSeries'
  s.files = Dir['examples/**/*.rb'] + 
            Dir['examples/**/*.cfg'] + 
            Dir['doc/**/*.*'] + 
            Dir['lib/**/*.rb'] + 
            ['lib/wmq/wmq.so', 'tests/test.rb', 'README', 'LICENSE'] 
  s.rubyforge_project = 'rubywmq'
  s.test_file = 'tests/test.rb'
  s.has_rdoc = false
  s.required_ruby_version = '>= 1.8.3'
  s.requirements << 'WebSphere MQ v5.3, v6 or v7 Client or Server'
end
if $0 == __FILE__
  Gem::Builder.new(spec).build
end