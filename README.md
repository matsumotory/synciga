# File Syncing Tool for NAT

## How to build

- optional

```
yum -y install nss-devel gtk2-devel alsa-lib-devel
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
cp -p out/Release/synciga ${HOME_DIR}/bin/.
synciga --sync-dir=./syncer/ myaccount@gmail.com
```

- Getting my Full JID from login information

```
$ synciga --sync-dir=./syncer/ myaccount@gmail.com
Directory: ./syncer/
Password:
Connecting... OK
Logging in... OK
Logged in... OK
Assigned FullJID myaccount@gmail.com/synciga********
Input below command on client synciga

synciga --sync --remote-dir=./syncer/ myaccount@gmail.com myaccount@gmail.com/synciga********
```

### Run syncer client using Gmail account at any other place

- Login google talk server and input password for Gmail Account

```
cp -p out/Release/synciga ${HOME_DIR}/bin/
synciga --sync --remote-dir=./syncer/ myaccount@gmail.com myaccount@gmail.com/synciga********
```

or

```
synciga --sync --remote-dir=./syncer/ --sync-dir=./test_sync/ myaccount@gmail.com myaccount@gmail.com/synciga********
```

- create file into ${HOME_DIR}

```
echo hoge >> ./test_sync/hoge.txt
```

The file is transfered to ./syncer/hoge.txt of the server at home under NAT.

Very coool.

## License
under the MIT License:

* http://www.opensource.org/licenses/mit-license.php

