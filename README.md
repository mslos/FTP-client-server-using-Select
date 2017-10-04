# To run
Running basic server and client from command line:

```
./obj/FTPserver 127.0.0.1 5000
./obj/FTPclient 127.0.0.1 5000
```
|Username:|Passowrds:|
|---------|----------|
|Nabil    |    1234|
|Brooke  |     qwer|
|Martin |     iluvnet|
|Yasir	|	 ethernet|
|Stefan|	     ~!:?|

Note: Same username should not connect from two clients simultaneously.

To Debug:
```
strace [COMMAND TO RUN THE CLIENT/SERVER - SAME AS ABOVE]
```


# Final Submission


Commands supported:
## USER
Once the server starts, 1 or more clients can connect and are then prompted to authenticate themselves. The client is prompted for a username, and if the username is in the authenticated list, the server respons "Username OK, password required".

## PASS
If the user entered a valid user, and a valid password then the user is authenticated. 

## LS / PWD / CD
If the user is authenticated, they can use the above commands in a similar way that bash works. The server keeps current working directory for each of the users.

## !LS / !PWD / !CD
These are local commands that do not require authentication.


## PUT
Authenticated users can upload files to the server. The server only lets authenticated users connect. The server can handle muplitple simultaneous uploads by users via select(). 

## GET
Authenticated users can GET files from the server. Unlike PUT, the implementation of GET is blocking, so that if the user downloads a very large file, the server will not work for other users.

## NOTE on PUT/GET: 
Correctness of connection and security currently relies on temporal exlusion of multiple clients. Because of the two-phase handshake, no two clients can enter the authentication loop at the same time. Therefore, we can tell with reasonable certainty who a user is. We understand that a *security* problem may arise if a malicious client tries to constantly poll the port for an opening. Hashing the control fd, and saving it as cookies would address this edge case.

Enclosed are two testing files that we run simultaneously by piping them into stin. There are some irrgularities with GET that we experienced in a few cases, most likely due to file descriptor clashes.
We fixed the issue with gets from checkpoint 1.
