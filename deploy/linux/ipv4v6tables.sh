# 1 v4, ip6tables - syntax the same
sudo iptables -A INPUT -m conntrack --ctstate ESTABLISHED,RELATED -j ACCEPT
sudo iptables -A INPUT -p tcp --dport 22 -j ACCEPT
sudo iptables -A INPUT -p tcp --dport 31415 -j ACCEPT
sudo iptables -I INPUT 1 -i lo -j ACCEPT
sudo iptables -P INPUT DROP

sudo ip6tables -A INPUT -m conntrack --ctstate ESTABLISHED,RELATED -j ACCEPT
sudo ip6tables -A INPUT -p tcp --dport 22 -j ACCEPT
sudo ip6tables -A INPUT -p tcp --dport 31415 -j ACCEPT
sudo ip6tables -I INPUT 1 -i lo -j ACCEPT
sudo ip6tables -P INPUT DROP

# 2 install persistent
# apt-get install iptables-persistent

# 3 make persistent
# iptables-save > /etc/iptables/rules.v4

# ipv6 the same
# ip6tables-save > /etc/iptables/rules.v6
