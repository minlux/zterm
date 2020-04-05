# zterm
Serial terminal server used to request a zmodem file transfer to the remote host. Therefore the client has to establish a *115200 8n1* terminal connection to the server.

List of available files can be displayed with:
```
zdir
```

Files are requested by command:
```
zsend *file* + CR
```

This server terminates, on reception of command:
```
CLIENT
```

The actuall file transfer is based on the invocation of a shell script *zsend* which must be located parallel to the *zterm* binary. The shell script *zsend*
gets the requested file as first argument. It looks for that file in an subfolder `zdata`. \
*zsend* itself is using the linux command line tool "sz" for the transfer (which is part of the lszrz package, https://ohse.de/uwe/software/lrzsz.html).

## Build
The build process of "zterm" is based on cmake:
```
cd build
cmake ../src
make
```


## Installation
Copie the files *zterm* and *zsend* as well as the folder `zdata` to destination. Ensure that *zterm* and *zsend* are executable. Otherwise do it ...
```
sudo chmod 755 ./zterm
sudo chmod 755 ./zsend
```

## Usage
Place the files to be served by *zterm* into folder `zdata` and start start *zterm*
like this:
```
./zterm
```

