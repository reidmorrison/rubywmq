call "C:\Program Files\Microsoft Visual Studio\VC98\Bin\VCVARS32.BAT"
ruby extconf.rb --with-mqm-include="C:\Program Files\IBM\WebSphere MQ\Tools\c\include" --with-mqm-lib="."
nmake
