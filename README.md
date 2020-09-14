# xv6 for internet os
This OS, called InternetOS, is forked from xv6 which is a re-implementation of Dennis Ritchie's and Ken Thompson's Unix
Version 6 (v6) by MIT. 

## Purpose of InternetOS
InternetOS has a kernel that uses resources such as CPU, memory, and processes transparently, regardless of software and hardware resources.
But xv6 which is a basis of InternetOS has no function related transparency. So now, InternetOS is implementing experimental functions such as TCP/IP stack, 9P protocol.
In the future, InternetOS will be possible to use resources of remote computers as if there are resources on your computer.

## Roadmap
- [x] TCP/IP stack
- [x] 9P Server(User program)
- [ ] 9P Client(User program)
- [ ] Network File System in kernel
- [ ] transparency file & disk
- [ ] transparency memory
