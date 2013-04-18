# File Syncing Tool for NAT

## How to build

- optional

```
yum -y install nss-devel gtk2-devel alsa-lib-devel.x86_64
```

- build

```
git submodule init
git submodule update
cd src
sh update_makefile.sh
make
```

## How to use
### Run server using Gmail account at home under NAT

- Login google talk server and input password for Gmail Account

```
cp -p out/Release/synciga ${REMOTE_HOME_DIR}/.
cd ${REMOTE_HOME_DIR}
./synciga myacount@gmail.com
```

- Getting my Full JID from login information

```
$ ./out/Release/synciga myaccount@gmail.com
Directory: /tmp/synciga
Password:
connecting...
logging in...
logged in...
Logged in as myaccount@gmail.com/synciga*******  < Full JID
```

### Run syncer client using Gmail account at any other place

- Login google talk server and input password for Gmail Account

```
cp -p out/Release/synciga ${HOME_DIR}
./synciga myacount@gmail.com myacount@gmail.com/synciga*******
```

- create file into ${HOME_DIR}

```
echo hoge >> ${HOME_DIR}/hoge.txt
```

The file is transfered to ${REMOTE_HOME_DIR}/hoge.txt of the server at home under NAT.

Very coool.

## License
under the MIT License:

* http://www.opensource.org/licenses/mit-license.php

