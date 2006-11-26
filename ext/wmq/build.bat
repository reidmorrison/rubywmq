call "C:\Program Files\Microsoft Visual Studio\VC98\Bin\VCVARS32.BAT"
ruby ../../generate/generate_reason.rb "C:\Program Files\IBM\WebSphere MQ\Tools\c\include"
ruby ../../generate/generate_const.rb "C:\Program Files\IBM\WebSphere MQ\Tools\c\include"
ruby ../../generate/generate_structs.rb "C:\Program Files\IBM\WebSphere MQ\Tools\c\include"
copy "C:\Program Files\IBM\WebSphere MQ\Tools\lib\mqm.lib"
copy "C:\Program Files\IBM\WebSphere MQ\Tools\lib\mqic32.lib"
ruby extconf.rb --with-mqm-include="C:\Program Files\IBM\WebSphere MQ\Tools\c\include" --with-mqm-lib="."
nmake
ruby extconf_client.rb --with-mqm-include="C:\Program Files\IBM\WebSphere MQ\Tools\c\include" --with-mqm-lib="."
nmake
