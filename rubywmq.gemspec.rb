require 'rubygems'
spec = Gem::Specification.new do |s|
  s.name = 'rubywmq'
  s.version = '0.1.0'
  s.author = 'Reid Morrison'
  s.email = 'rubywmq@gmail.com'
  s.homepage = 'http://rubywmq.rubyforge.org'
  s.platform = Gem::Platform::RUBY
  s.summary = 'A Ruby interface into WebSphere MQ (MQSeries)'
  s.files = File.readlines('Manifest.txt').map { |l| l.chomp }
  s.require_path = 'lib/wmq'
  s.extensions = ['ext/wmq/extconf.rb', 'ext/wmq/extconf_client.rb']
  s.test_file = 'tests/test.rb'
  s.has_rdoc = true
  s.extra_rdoc_files = ['README', 'LICENSE']  
end
if $0 == __FILE__
  Gem::manage_gems
  Gem::Builder.new(spec).build
end