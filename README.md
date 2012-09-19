## rubywmq

* http://github.com/reidmorrison/rubywmq

### Description

RubyWMQ is a high performance native Ruby interface into WebSphere MQ.

### Compatibility

Ruby

* RubyWMQ only works on Ruby MRI on with Ruby 1.8.7, Ruby 1.9.3, or greater
* For JRuby, see http://github.com/reidmorrison/jruby-jms

WebSphere MQ

* RubyWMQ parses the header files that come with WebSphere MQ so always
  stays up to date with the latest structures and return codes
* RubyWMQ has been tested against WebSphere MQ 5, 6, and 7.5

## Example

    require 'rubygems'
    require 'wmq'

    # Connect to a local queue manager called 'TEST' and put a single message
    # on the queue 'TEST.QUEUE'
    WMQ::QueueManager.connect(:q_mgr_name=>'TEST') do |qmgr|
      qmgr.put(:q_name=>'TEST.QUEUE', :data => 'Hello World')
    end

## Installation

### Installing on UNIX/Linux

* Install a 'C' Compiler, GNU C++ is recommended
* Install Ruby using the package manager for your platform
* Also install RubyGems and ruby-dev or ruby-sdk packages if not already installed
* Install the [WebSphere MQ Client and/or Server](﻿http://www.ibm.com/developerworks/downloads/ws/wmq/)

    Note: Install the Development Toolkit (SDK) and Client

* Install RubyWMQ Gem

    gem install rubywmq

If no errors appear RubyWMQ is ready for use

#### Installation Errors

Use this command to find the directory in which the gem was installed

    gem which rubywmq

When WebSphere MQ is not installed in the default location, call the build
command directly and supply the location explicitly:

    ruby extconf.rb --with-mqm-include=/opt/mqm/inc --with-mqm-lib=/opt/mqm/lib
    make

For platforms such as AIX and HP-UX it may be necessary to statically link in
the WebSphere MQ client library when the auto-detection build above does not work.
This build option is a last resort since it will only work using a client connection

    ruby extconf_client.rb --with-mqm-include=/opt/mqm/inc --with-mqm-lib=/opt/mqm/lib
    make

### Installing on Windows

#### Install Ruby and DevKit

* Download and install the Ruby installer from http://rubyinstaller.org/downloads/

    Select "Add Ruby executables to your PATH" during the installation

* Download and install the Development Kit from the same site

    Extract files into c:\DevKit

* Open a command prompt and run the commands below:

    cd c:\DevKit
    ruby dk.rb init

If you experience any difficulties, see ﻿https://github.com/oneclick/rubyinstaller/wiki/Development-Kit

#### Install WebSphereMQ

* Install the [WebSphere MQ Client and/or Server](﻿http://www.ibm.com/developerworks/downloads/ws/wmq/)

    Note: Install the Development Toolkit (SDK) and Client

#### Install the RubyWMQ Gem

    call "c:\DevKit\devkitvars.bat"
   ﻿ gem install rubywmq --platform=ruby

#### Installation Errors

Use this command to find the directory in which the gems are installed

    gem env

The path to the rubywmq gem will be something like

    C:\Ruby193\lib\ruby\gems\1.9.1\gems\rubywmq-2.0.0\ext

When WebSphere MQ is not installed in the default location, change to the directory
above and call the build command directly while supplying the location explicitly:

    call "C:\DevKit\devkitvars.bat"
    ruby extconf.rb --with-mqm-include="C:\Program Files\IBM\WebSphere MQ\Tools\c\include"
    nmake

## Verifying the build

### Verifying a local WebSphere MQ Server installation

* Create a local Queue Manager called TEST. Select the option to create the server
side channels.
* Create a local queue called TEST.QUEUE
* Run the following Ruby Code in an irb session:

    require 'rubygems'
    require 'wmq'
    WMQ::QueueManager.connect(:q_mgr_name=>'TEST') do |qmgr|
      qmgr.put(:q_name=>'TEST.QUEUE', :data => 'Hello World')
    end

## Rails Installation

After following the steps above to compile the source code, add the following
line to Gemfile

    gem 'rubywmq', :require => 'wmq'

## Architecture

RubyWMQ uses an automatic detection library to figure out whether a Client or Server
is installed locally. This prevents issues with having to statically link with
both the client and server libraries and then having to select the "correct"
one on startup.

Additionally, this approach allows RubyWMQ to be simultaneously connect to both a local
Queue Manager via server bindings and to a remote Queue Manager using Client bindings.

Instead of hard coding all the MQ C Structures and return codes into RubyWMQ, it
parses the MQ 'C' header files at compile time to take advantage of all the latest
features in new releases.

## Contributing

Once you've made your great commits:

1. [Fork](http://help.github.com/forking/) rubywmq
2. Create a topic branch - `git checkout -b my_branch`
3. Push to your branch - `git push origin my_branch`
4. Create an [Issue](http://github.com/reidmorrison/rubywmq/issues) with a link to your branch
5. That's it!

## Meta

* Code: `git clone git://github.com/reidmorrison/rubywmq.git`
* Home: <http://github.com/reidmorrison/rubywmq>
* Bugs: <http://github.com/reidmorrison/rubywmq/issues>
* Gems: <http://rubygems.org/gems/rubywmq>

This project uses [Semantic Versioning](http://semver.org/).

## Author

Reid Morrison :: reidmo@gmail.com :: @reidmorrison

Special thanks to Edwin Fine for the RFH parsing code and for introducing me to
the wonderful world of Ruby

## License

Copyright 2006 - 2012 J. Reid Morrison

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
