## rubywmq

RubyWMQ is a high performance native Ruby interface into WebSphere MQ.

* http://github.com/reidmorrison/rubywmq

### Features

The Ruby WMQ interface currently supports the following features:

High performance

* Able to read over 2000 messages per second from a Queue
* (Non-persistent messages, < 4K each, MQ V6, running on Windows Laptop)

Full support for the entire MQ Administration interface (MQAI)

* Create Queues
* Query Queue Depths
* etc…

Full support for all WebSphere MQ Headers

* Rules and Format Header 2 (RFH2)
* Rules and Format Header (RFH)
* Name Value pairs returned as a Hash
* Dead Letter Header
* Transmission Queue Header
* IMS, CICS, …..

Conforms with the Ruby way. Implements:

* each
* Code blocks

Relatively easy interface for reading or writing messages

* MQ Headers made easy

Single Ruby WMQ auto-detection library that concurrently supports:

* WebSphere MQ Server Connection
* WebSphere MQ Client Connection

Includes latest client connection options such as SSL

Tested with WebSphere MQ V5.3, V6, V7, and V7.5

Is written in C to ensure easier portability and performance

### Compatibility

Ruby

* RubyWMQ only works on Ruby MRI on with Ruby 1.8.7, Ruby 1.9.3, or greater
* For JRuby, see http://github.com/reidmorrison/jruby-jms

WebSphere MQ

* RubyWMQ parses the header files that come with WebSphere MQ so always
  stays up to date with the latest structures and return codes
* RubyWMQ has been tested against WebSphere MQ 5, 6, and 7.5

## Example

```ruby
require 'rubygems'
require 'wmq'

# Connect to a local queue manager called 'TEST' and put a single message
# on the queue 'TEST.QUEUE'
WMQ::QueueManager.connect(:q_mgr_name=>'TEST') do |qmgr|
    qmgr.put(:q_name=>'TEST.QUEUE', :data => 'Hello World')
end
```

## More Examples

There are many examples covering many of the ways that RubyWMQ can be used. The examples
are installed as part of the Gem under the 'examples' sub-directory. The examples can
also be be viewed at https://github.com/reidmorrison/rubywmq/tree/master/examples

Put one message to a Queue (Without opening the queue)

* put1_a.rb
* put1_b.rb
* put1_c.rb

Put messages to a Queue

* put_a.rb
* put_b.rb

Read one message from a queue

* get_a.rb

Reading Several messages from a Queue:

* each_a.rb
* each_b.rb
* each_header.rb

Connect using MQ Client connection

* get_client.rb

Put Messages to a Queue as a group

* put_group_a.rb
* put_group_b.rb

Put Messages to a Queue, including message headers

* put_dlh.rb
* put_dynamic_q.rb
* put_rfh.rb
* put_rfh2_a.rb
* put_rfh2_b.rb
* put_xmit_q.rb

Writing multiple files to a queue, where each file is a separate message:

* files_to_q.rb, files_to_q.cfg

Writing the contents of a queue to multiple files, where each message is a separate file:

* q_to_files.rb, q_to_files.cfg

Sample “client” and “server” side applications for sending or processing requests

* request.rb
* server.rb

## Documentation

Documentation for the RubyWMQ Gem is generated automatically when the gem is installed.
It is also available [online](http://rubywmq.rubyforge.org/doc/index.html)

## Installation

### Installing on UNIX/Linux

Install a 'C' Compiler, GNU C++ is recommended

Install Ruby using the package manager for your platform

Also install RubyGems and ruby-dev or ruby-sdk packages if not already installed

Install the [WebSphere MQ Client and/or Server](﻿http://www.ibm.com/developerworks/downloads/ws/wmq/)

* Note: Install the Development Toolkit (SDK) and Client

Install RubyWMQ Gem

    gem install rubywmq

If no errors appear RubyWMQ is ready for use

#### Installation Errors

Use this command to find the directory in which the gems are installed

    gem env

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

Download and install the Ruby installer from http://rubyinstaller.org/downloads/

* Select "Add Ruby executables to your PATH" during the installation

Download and install the Development Kit from the same site

* Extract files into c:\DevKit

Open a command prompt and run the commands below:

    cd c:\DevKit
    ruby dk.rb init

If you experience any difficulties, see ﻿https://github.com/oneclick/rubyinstaller/wiki/Development-Kit

#### Install WebSphereMQ

Install the [WebSphere MQ Client and/or Server](﻿http://www.ibm.com/developerworks/downloads/ws/wmq/)

* Note: Install the Development Toolkit (SDK) and Client

#### Install the RubyWMQ Gem

    call "c:\DevKit\devkitvars.bat"
    gem install rubywmq --platform=ruby

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

Create a local Queue Manager called TEST. Select the option to create the server
side channels.

Create a local queue called TEST.QUEUE

Run the following Ruby Code in an irb session:

```ruby
require 'rubygems'
require 'wmq'
WMQ::QueueManager.connect(:q_mgr_name=>'TEST') do |qmgr|
  qmgr.put(:q_name=>'TEST.QUEUE', :data => 'Hello World')
end
```

## Rails Installation

After following the steps above to compile the source code, add the following
line to Gemfile

    gem 'rubywmq'

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

## FAQ

### Programs fail with: `require': no such file to load -- wmq (LoadError)

After successfully installing RubyWMQ using the command “gem install rubywmq”,
program fails with output similar to the following:

    rubywmq-0.3.0/tests/test.rb:4:in `require': no such file to load -- wmq (LoadError)
        from rubywmq-0.3.0/tests/test.rb:4

Answer: Add the following line to the top of your Ruby program

    require 'rubygems'

### Program fails to connect with MQRC2059, MQRC_Q_MGR_NOT_AVAILABLE

When connecting to either a local or remote WebSphere MQ Queue Manager, a very common
error returned is that the Queue Manager is not available. This error can occur
under any of the following circumstances:

#### MQ Server Connections (Local Queue Manager)

Possible Configuration Issues:

* Ensure that :connection_name is not being supplied to the connect method.
Even if it is supplied with a nul or empty value, it will cause a client connection attempt to be made.

Is the Queue Manager active?

* Try running the following command on the machine running the Queue Manager
and check that the Queue Manager is marked as ‘Running’:

```
    dspmq

    Expected output:
    QMNAME(REID)                          STATUS(Running)
```

* Check that the :q_mgr_name supplied to QueueManager::connect matches the Queue Manager name above.
* Note: Queue Manager names are case-sensitive

#### MQ Client Connections (Remote Queue Manager)

Possible Client Configuration Issues:

* Incorrect host name
* Incorrect port number
* Incorrect Channel Name

    For example, the channel being used does not exist on the remote Queue Manager.

* Note: The channel name is case-sensitive

Incorrect Queue Manager Name

* :q_mgr_name is optional for Client Connections. It does however ensure that the program connects to the expected Queue Manager.

    For example when the wrong Queue Manager listener is now running on the expected port.

* Is the MQ listener program running on the port supplied above?


On UNIX/Linux, try the following command on the machine running the Queue Manager:

    ps -ef | grep runmqlsr

* The Queue Mananger name and port number should be displayed

* If no port number is specified on the command line for an instance of runmqlsr, it means that it is using port 1414.

Is the Queue Manager active?

* Try running the following command on the machine running the Queue Manager and check that the Queue Manager is marked as ‘Running’:

```
    dspmq

    Expected output:
    QMNAME(REID)                          STATUS(Running)
```

* Check that the :q_mgr_name supplied to QueueManager::connect matches the Queue Manager name above. Note: Queue Manager names are case-sensitive
* Check if the Channel being used is still defined on the Queue Manager

    On the machine running the Queue Manager, run the following commands (may need to run them under the 'mqm' userid):

```
    runmqsc queue_manager_name
    dis channel(*) chltype(SVRCONN)
```

* Replace queue_manager_name above with the actual name of the Queue Manager being connected to
* Look for the channel name the application is using.
* Note: the channel name is case-sensitive.

## Support

Ruby WMQ Community Support Mailing List:

    http://rubyforge.org/mailman/listinfo/rubywmq-misc

Feature and Bug Reports: <http://github.com/reidmorrison/rubywmq/issues>

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
