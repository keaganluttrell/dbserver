# Basic File DB with server

This implements a few basic protocols that build on the [basic-file-db](https://github.com/keaganluttrell/basic-file-db) project.

### Build 

```bash
make -s

```

### Start the server

```bash

./bin/dbserver -f foo.db -n -p 8080
```

While server is running make requests from client in another terminal

### Simple Client Requests

```bash
./bin/dbcli -h 127.0.0.1 -p 8080 -a "tim,1 main st,80"

./bin/dbcli -h 127.0.0.1 -p 8080 -l

./bin/dbcli -h 127.0.0.1 -p 8080 -r "tim"

./bin/dbcli -h 127.0.0.1 -p 8080 -l
```


