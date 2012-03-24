#! /usr/bin/env python
"""
Serves files out of its current directory.
Doesn't handle POST requests.
"""
import SocketServer
import SimpleHTTPServer
import string
import math
import random

PORT = 8000

x1 = 0
x2 = 1

def move():
  """ sample function to be called via a URL"""
  return 'hi'


def proc():
  """Return cpu user time"""
  with open('/proc/stat') as file:
    cpu = string.split(file.readline())
    return cpu[1]


def fib():
  """Return next Fibonacci number """
  global x1, x2
  y = x1 + x2
  x1 = x2
  x2 = y
  return y


def rand():
  return random.randint(0,10)

x = 0;
def sine():
  """Return a sequence based on sine"""
  global x;
  x += 0.3;
  return 5 * math.sin(x) + 10;


class CustomHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):
  def reply(self, x):
    self.send_response(200)
    self.send_header('Content-type','text/html')
    self.end_headers()
    self.wfile.write(x)
    return

  def do_GET(self):
    #Sample values in self for URL: http://localhost:8080/jsxmlrpc-0.3/
    #self.path  '/jsxmlrpc-0.3/'
    #self.raw_requestline   'GET /jsxmlrpc-0.3/ HTTP/1.1rn'
    #self.client_address  ('127.0.0.1', 3727)
    if self.path=='/move':
      #This URL will trigger our sample function and send what it returns back to the browser
      self.reply(move()) #call sample function here
      return
    elif self.path == '/proc':
      self.reply(proc())
      return
    elif self.path == '/fib':
      self.reply(fib())
      return
    elif self.path == '/sine':
      self.reply(sine())
      return
    elif self.path == '/rand':
      self.reply(rand())
      return
    else:
      #serve files, and directory listings by following self.path from
      #current working directory
      SimpleHTTPServer.SimpleHTTPRequestHandler.do_GET(self)

httpd = SocketServer.ThreadingTCPServer(('', PORT),CustomHandler)

print "serving at port", PORT
httpd.serve_forever()
