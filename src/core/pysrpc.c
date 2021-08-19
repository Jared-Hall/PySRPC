/*
 * Copyright (c) 2013, Court of the University of Glasgow
 * All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * - Neither the name of the University of Glasgow nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

 /*
* Programmed by: Jared Hall
* Contact me if any questions: jhall10@cs.uoregon.edu
* Description: This is the c-source code for a python extension of SRPC.
*              It's a bit (alot) faster to use use an extension for SRPC than to
*              implement the entire protocol in python since python is slower.
*              Instead of doing all of that we leave most of the work to C and
*              just provide an extension of it's functionality to python.
*/
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "srpc/srpc.h"

#define SRPC_RECV_LEN 65535
#define SRPC_RESP_LEN 256


//======================= SRPC Python extension ================================
typedef struct {
  PyObject_HEAD
  RpcConnection rpc;
  RpcService rps;
  RpcEndpoint sender;
  char *otherHost;
  char *otherService;
  int otherPort;
  char *myHost;
  char *myService;
  int myPort;
} SRPCObject;

static void SRPC_dealloc(SRPCObject *self) {
  Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyObject *SRPC_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
  SRPCObject *self;
  self = (SRPCObject *) type->tp_alloc(type, 0);
  if(self != NULL) {
    self->otherPort = 0;
    self->myPort = 0;
  }
  return (PyObject *) self;
}

static PyMemberDef SRPC_members[] = {
  {"otherHost", T_STRING, offsetof(SRPCObject, otherHost), 0, "IP Address of other"},
  {"otherService", T_STRING, offsetof(SRPCObject, otherService), 0, "Service to connect to"},
  {"otherPort", T_INT, offsetof(SRPCObject, otherPort), 0, "Access port of other"},
  {"myHost", T_STRING, offsetof(SRPCObject, myHost), 0, "IP Address of my server"},
  {"myService", T_STRING, offsetof(SRPCObject, myService), 0, "Service to offer"},
  {"myPort", T_INT, offsetof(SRPCObject, myPort), 0, "Access port of my server"},
  {NULL} //Sentinel
};

//=============================== Base wrappers ================================
/* Description: This section contains a collection of wrappers for the base SRPC
*                API.
*/
static PyObject *SRPC_init(SRPCObject *self, PyObject *args){
  /*
   * initialize RPC system - bind to �port� if non-zero
   * otherwise port number assigned dynamically
   * returns True if successful, False if failure
   */
   if(!PyArg_ParseTuple(args, "i", &self->myPort)) {
     return NULL;
   }
   if(! rpc_init(self->myPort)) {
     fprintf(stderr, "Failed to initialize SRPC for port: %d\n", self->myPort);
     Py_RETURN_FALSE;
   }
   Py_RETURN_TRUE;
}

static PyObject *SRPC_offer(SRPCObject *self, PyObject *args){
  /*
   * initialize RPC system - bind to �port� if non-zero
   * otherwise port number assigned dynamically
   * returns True if successful, False if failure
   */
   if(!PyArg_ParseTuple(args, "s", &self->myService)) {
     return NULL;
   }
   if(! (self->rps = rpc_offer(self->myService))) {
     fprintf(stderr, "ERROR: Failed to offer service: %s\n", self->myService);
     Py_RETURN_FALSE;
   }
   Py_RETURN_TRUE;
}

static PyObject *SRPC_rpcDetails(SRPCObject *self, PyObject *Py_UNUSED(ignored)) {
  /*
   * obtain our ip address (as a string) and port number
   */
  char *ipaddr = NULL; unsigned short int port;
  rpc_details(ipaddr, &port);
  return Py_BuildValue("(si)", ipaddr, port);
}

static PyObject *SRPC_reverselu(SRPCObject *self, PyObject *Py_UNUSED(ignored)) {
  /*
   * reverse lookup of ip address (as a string) to fully-qualified hostname
   */
  char *ipaddr = NULL, *hostname = NULL;
  rpc_reverselu(ipaddr, hostname);
  return Py_BuildValue("(ss)", ipaddr, hostname);
}

static PyObject *SRPC_connect(SRPCObject *self, PyObject *args, PyObject *kwds) {
  /*
   * send connect message to host:port with initial sequence number
   * svcName indicates the offered service of interest
   * returns True after target accepts connect request
   * else returns False (failure)
   */
  static char *kwlist[] = {"host", "port",  "service", "seqNo", NULL};
  long seqno;
  if(!PyArg_ParseTupleAndKeywords(args, kwds, "|sisi", kwlist, &self->otherHost, &self->otherPort, &self->otherService, &seqno)) {
    return NULL;
  }

  self->rpc = rpc_connect(self->otherHost, self->otherPort, self->otherService, seqno);
  if (self->rpc == NULL) {
      Py_RETURN_FALSE;
  }
  Py_RETURN_TRUE;
}

static PyObject *SRPC_call(SRPCObject *self, PyObject *args) {

  const char *msg; char resp[SRPC_RESP_LEN]; unsigned len;
  if(!PyArg_ParseTuple(args, "s", &msg)) {
    return NULL;
  }
  int n = (int) strlen(msg) + 1;
  Q_Decl(query, n);
  sprintf(query, "%s", msg);

  if (! rpc_call(self->rpc, Q_Arg(query), n, resp, SRPC_RESP_LEN, &len)) {
    Py_RETURN_FALSE;
  }

  return PyUnicode_FromString(resp);
}

static PyObject *SRPC_disconnect(SRPCObject *self, PyObject *Py_UNUSED(ignored)) {
  /*
   * disconnect from RPC return None
   */
  rpc_disconnect(self->rpc);
  Py_RETURN_NONE;
}

static PyObject *SRPC_withdraw(SRPCObject *self, PyObject *Py_UNUSED(ignored)) {
  /*
   * disconnect from RPC return None
   */
  rpc_withdraw(self->rps);
  Py_RETURN_NONE;
}

static PyObject *SRPC_query(SRPCObject *self, PyObject *Py_UNUSED(ignored)) {
  /*
  THIS HAS A PROBLEM: Python won't allow threads from c-process to operate in tandem
  without escaping the GIL (this includes SRPC threads). I escape the GIL in the
  code below but now the query is no longer in python's view.
  Since the query blocks until the transaction is complete it is impossible to handle
  multiple queries from a single object at the same time.
  The only way to do this single thread async
  would be to rewrite the C-query method to be non-blocking.
  Otherwise, both query and response must be sync or python bricks. T.T

  Temp solution: I lost many hairs wrangling with this.
                 In my code both entities are clients and
                 servers so a standard ACK is fine for the imediate resp. Then we
                 can do a call to send the actual response in the future.
                 This effectivly makes queries in srpc non-blocking, single thread,
                 async but with alot of added complexity since the client has
                 to be both client and server. It's kinda trash, but it works.
  */
  char msg[SRPC_RECV_LEN];
  unsigned len;
  Py_BEGIN_ALLOW_THREADS
  if ((len = rpc_query(self->rps, &self->sender, msg, SRPC_RECV_LEN)) < 1) {
      Py_RETURN_FALSE;
  } else {
      msg[len] = '\0';
  }
  Py_END_ALLOW_THREADS
  return Py_BuildValue("s", msg);
}

static PyObject *SRPC_response(SRPCObject *self, PyObject *args) {

  char resp[SRPC_RESP_LEN]; const char *msg; unsigned rlen;
  if(!PyArg_ParseTuple(args, "s", &msg)) {
    return NULL;
  }
  sprintf(resp, "%s", msg);
  rlen = (unsigned) strlen(resp) + 1;
  if(rpc_response(self->rps, &self->sender, resp, rlen)){
    Py_RETURN_TRUE;
  } else {
    Py_RETURN_FALSE;
  }
}

//==============================================================================


static PyMethodDef SRPCMethods[] = {
  {"init", (PyCFunction) SRPC_init, METH_VARARGS, "Initialize SRPC connection"},
  {"offer", (PyCFunction) SRPC_offer, METH_VARARGS, "Initialize SRPC connection"},
  {"details", (PyCFunction) SRPC_rpcDetails, METH_NOARGS, "Get the ip address and port"},
  {"reverse_lookup", (PyCFunction) SRPC_reverselu, METH_NOARGS, "Get the ip address and hostname"},
  {"connect", (PyCFunction) SRPC_connect, METH_VARARGS | METH_KEYWORDS, "Connect to RPC server specified by <host>:<port> using <service>"},
  {"listen", (PyCFunction) SRPC_query, METH_NOARGS, "listen for messages from client"},
  {"respond", (PyCFunction) SRPC_response, METH_VARARGS, "Send an RPC response to the client"},
  {"call", (PyCFunction) SRPC_call, METH_VARARGS, "Send an RPC query to the connected server"},
  {"disconnect", (PyCFunction) SRPC_disconnect, METH_NOARGS, "Disconnect from RPC server"},
  {"withdraw", (PyCFunction) SRPC_withdraw, METH_NOARGS, "withdraw SRPC service"},
  {NULL, NULL, 0, NULL}
};

static PyTypeObject SRPCType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = "srpc.SRPC",
  .tp_doc = "An SRPC object",
  .tp_basicsize = sizeof(SRPCObject),
  .tp_itemsize = 0,
  .tp_flags = Py_TPFLAGS_DEFAULT,
  .tp_new = SRPC_new,
  .tp_dealloc = (destructor) SRPC_dealloc,
  .tp_members = SRPC_members,
  .tp_methods = SRPCMethods
};

//================ CCM Module Definition ==============
static struct PyModuleDef srpcmodule = {
  PyModuleDef_HEAD_INIT,
  "srpc", //Name of module
  NULL, //Module docs
  -1, //size of per inter state of module
};

//============== CCM Module Initialization ============
PyMODINIT_FUNC PyInit_srpc(void) {
  PyObject *module;

  if(PyType_Ready(&SRPCType) < 0) {
    return NULL;
  }

  module = PyModule_Create(&srpcmodule);
  if(module == NULL) {
    return NULL;
  }

  Py_INCREF(&SRPCType);
  if(PyModule_AddObject(module, "SRPC", (PyObject *) &SRPCType) < 0) {
    Py_DECREF(&SRPCType);
    Py_DECREF(module);
    return NULL;
  }
  //register srpc errors here

  return module;
}
