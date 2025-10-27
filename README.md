# RemoteDesktopService
Currently a means of sharing a screen from one laptop to another. Writen in C++ with a TCP connection and support only for linux.

When compiling use -lX11.

+------+---------+---------+---------------+
| dir  | visuals | control | file transfer |
+-----+---------+----------+---------------+
| lite |   ✅️    |    ❌️   |       ❌️      | 
+------+---------+---------+---------------+
| func |   ✅️    |    ✅️   |       ❌️      | 
+------+---------+---------+---------------+
| full |   ✅️    |    ✅️   |       ✅️      | 
+------+---------+---------+---------------+