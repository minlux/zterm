# zterm
Linux base *serial terminal server* used to provide a simple way to transfer files to any kind of client, using zmodem protocol.

![Hypter Terminal](doc/greetings.png)

## Usecase
Imagine you are on a computer or an embedded device and you need to get files from a linux machine. However you don't have any network or data drive to get these files. The only interface which is available is a serial (RS232, UART) interface. Then *zterm* may be for you!

*zterm* is a little tool that is intended to run on the linux machine. It receives commands from the remote computer via RS232 to
- send a list of files ("served" by the server)
- and initiate a *zmodem* file transfer ("up" to the remote)

On the remote site (the client) you only need a terminal program that "understands" the *zmodem protocol*. On Windows *Hyper Terminal* is such a program.

![Hypter Terminal](doc/zdir.png)

With the command `zdir` + *ENTER* client can request a list of files on the server.\
With the command `zsend <file>` + *ENTER* the client can instruct the server to send the requested file.

![Hypter Terminal](doc/zsend.png)



## Build
The build process of *zterm* is based on cmake:
```
cd build
cmake ..
make
```

## Usage
### Server
Before you can use *zterm* you have to provide a "link" to the data served by *zterm*. *zterm* expects a folder `zdata` next to its executable containing the data. Using a symbolic link is the recommended solution. E.g.:
```
cd build
ln -s ~/Downloads zdata
```

Currently *zterm* is hard coded to use `/dev/ttyUSB0`. Its operates on *9600bps 8N1*. To start it you just has to call
```
./zterm
```
from the `build` directory.

### Client
On the client side you can use whatever terminal program you want. You just has to connect to the serial interface with *9600bps 8N1*. Then you enter
- `zdir` + *ENTER* to get a file list
- `zsend <file>` + *ENTER* to request the file transfer. (then, on Windows *Hyper Terminal* for example, a popup window will appear and "receive" the file automatically)

Normally the server runs in an "endless loop". However, if the client wants to stop the server it just has to type `CLIENT`. Then the server will terminate and no further interaction is possible.


## Internals
The actuall file listing is based on the invocation of a shell script *zdir* which must be located parallel to the *zterm* binary. The actuall file transfer is based on the invocation of a shell script *zsend* which must be located parallel to the *zterm* binary. Both must be executable!

The shell script *zsend* gets the requested file as first argument. It looks for that file in an subfolder `zdata`. *zsend* itself is using the linux command line tool *sz* for the transfer (which is part of the lszrz package, https://ohse.de/uwe/software/lrzsz.html). Possibly you need to install that package first:
```
sudo apt-get install lrzsz
```

Why is that "strange" command *CLIENT* to quit the server? To make long story short: I used to use *zterm* in a script which started first *zterm* and afterwards *pppd*. *pppd* allows to establish a *direct cable connection* [DCC](https://en.wikipedia.org/wiki/Direct_cable_connection) between a windows computer and the linux machine. Windows initiates a *DDC* connection, by sending the command *CLIENT* a few times in advance. *zterm* is using this a trigger to terminate so that the following *pppd* can be started...
