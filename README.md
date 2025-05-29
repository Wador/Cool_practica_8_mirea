
1. client and server
  make
  make deb

2. install client and server
  sudo dpkg -i deb/myrpc-client_1.0-1_amd64.deb
  sudo dpkg -i deb/myrpc-server_1.0-1_amd64.deb
  sudo dpkg -i deb/libmysyslog_1.0-1_amd64.deb
  ```

3. setting server parametrs
  sudo mkdir -p /etc/myRPC
  echo -e "port=5555\nsocket_type=stream" | sudo tee /etc/myRPC/myRPC.conf
 

4.  user file
  echo "vboxuser" | sudo tee /etc/myRPC/users.conf
 
 5. star server
  sudo myrpc-server


6. TCP
  myrpc-client -h 127.0.0.1 -p 5555 -s -c "date"

