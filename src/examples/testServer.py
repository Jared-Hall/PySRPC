import srpc

EXIT = "q"

def initServer(rpc, port, service):
    #init SRPC server for the specified port
    if(not rpc.init(port)):
        print("Failed to initialize server at port: "+str(port)+"\n")
    elif(not rpc.offer(service)):
        print("Failed to offer service: "+service+"\n")

def listen(rpc):
    msg = ""
    print("Listening for queries from client... \n")
    while(msg != EXIT):
        msg = rpc.listen()
        print("[@server] Received: " + msg+"\n")
        rpc.respond(msg)

def main():
    #server info
    port = 5000
    service = "ECHO"

    #create an SRPC object
    rpc = srpc.SRPC()

    #initialize the server
    initServer(rpc, port, service)

    listen(rpc)
    rpc.withdraw()
    print("Goodbye!")


if(__name__=="__main__"):
    main()
