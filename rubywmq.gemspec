require 'rubygems'
require 'date'
spec = Gem::Specification.new do |s|
  s.name = 'rubywmq'
  s.version = '0.3.0'
  s.author = 'Reid Morrison'
  s.email = 'rubywmq@gmail.com'
  s.homepage = 'http://rubywmq.rubyforge.org'
  s.platform = Gem::Platform::RUBY
  s.summary = 'Ruby interface into WebSphere MQ (MQSeries)'
  s.files = Dir['examples/**/*.rb'] + File.readlines('Manifest.txt').map { |l| l.chomp }
  s.rubyforge_project = 'rubywmq'
  s.extensions << 'ext/extconf.rb'
  s.test_file = 'tests/test.rb'
  s.has_rdoc = true
  s.required_ruby_version = '>= 1.8.0'
  s.extra_rdoc_files = ['README', 'LICENSE']  
  s.requirements << 'WebSphere MQ, v5.3 or greater'
end
if $0 == __FILE__
  Gem::manage_gems
  Gem::Builder.new(spec).build
end