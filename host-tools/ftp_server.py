import os
from pyftpdlib.authorizers import DummyAuthorizer
from pyftpdlib.handlers import FTPHandler
from pyftpdlib.servers import FTPServer

def main():
    authorizer = DummyAuthorizer()
    authorizer.add_user('cam', 'cam', os.environ.get('A5100_FTPROOT', os.path.join(os.path.dirname(os.path.abspath(__file__)), 'ftp-root')), perm='elradfmwMT')
    handler = FTPHandler
    handler.authorizer = authorizer
    handler.passive_ports = range(60000, 60100)
    server = FTPServer(('0.0.0.0', 2121), handler)
    server.serve_forever()

if __name__ == '__main__':
    main()
