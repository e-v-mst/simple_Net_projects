Simple chat server written using epoll.

Each message sent by one client is sent to everyone else.
The client IP address is attached to the beginning of each message.
All clients receive messages about connecting new members and disconnecting existing ones.
