ruby ../../generate/generate_reason.rb /opt/mqm/inc
ruby ../../generate/generate_const.rb /opt/mqm/inc
ruby ../../generate/generate_structs.rb /opt/mqm/inc
ruby extconf.rb --with-mqm-include=/opt/mqm/inc --with-mqm-lib=/opt/mqm/lib
make
ruby extconf_client.rb --with-mqm-include=/opt/mqm/inc --with-mqm-lib=/opt/mqm/lib
make
