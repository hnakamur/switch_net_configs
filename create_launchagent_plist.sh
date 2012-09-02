#!/bin/bash
cat > ${HOME}/Library/LaunchAgents/net.naruh.switch_net_configs.plist <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>Label</key>
	<string>net.naruh.switch_net_configs</string>
	<key>ProgramArguments</key>
	<array>
		<string>${HOME}/switch_net_configs/ip_address_change_notifier</string>
		<string>${HOME}/switch_net_configs/switch_configs.rb</string>
	</array>
  <key>KeepAlive</key>
  <true/>
  <key>RunAtLoad</key>
  <true/>
	<key>OnDemand</key>
	<false/>
</dict>
</plist>
EOF
