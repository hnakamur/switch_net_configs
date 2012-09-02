all: ip_address_change_notifier

ip_address_change_notifier: ip_address_change_notifier.c
	cc -g -framework Foundation -framework SystemConfiguration \
		-o ip_address_change_notifier ip_address_change_notifier.c

install: ip_address_change_notifier
	mkdir -p ${HOME}/switch_net_configs
	install -m 755 ip_address_change_notifier switch_configs.rb \
		${HOME}/switch_net_configs
	cp -r templates ${HOME}/switch_net_configs
	sudo chmod 666 /etc/hosts
	./create_launchagent_plist.sh
	launchctl load -w ${HOME}/Library/LaunchAgents/net.naruh.switch_net_configs.plist
	
clean:
	rm ip_address_change_notifier
