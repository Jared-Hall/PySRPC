import srpc

#server info
ipaddr = "127.0.0.1"
port = 5000
service = "ECHO"

#create an SRPC object
rpc = srpc.SRPC()

#init SRPC client
if(not rpc.init(0)):
    print("Failed to initialize client.\n")
else:
    print("Client initialized..\n")
#connect to server
if(rpc.connect(ipaddr, port, service, 0)):
    msg = ""
    while(msg != "q"):
        msg = input("Enter a message to send or 'q' to quit: ")
        resp = rpc.call(msg)
        print("[@client] Received: "+resp)
    rpc.disconnect()
else:
    print("Failed to connect to server. Exiting...\n")

print("Goodbye!")
