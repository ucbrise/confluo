import env
import subprocess
import sys
import time
import unittest
from dialog import dialog_client


class test_dialog_client(unittest.TestCase):
    SERVER_EXECUTABLE = "dialogd"

    def start_server(self):
        self.server = subprocess.Popen([self.SERVER_EXECUTABLE])
        time.sleep(1)

    def stop_server(self):
        self.server.kill()

    def test_conncurrent_connections(self):
        self.start_server()
        clients = [dialog_client.dialog_client("127.0.0.1", 9090) for x in xrange(4)]

        for c in clients:
            c.disconnect()

        self.stop_server()


if __name__ == '__main__':
    if len(sys.argv) == 2:
        test_dialog_client.SERVER_EXECUTABLE = sys.argv.pop()
    else:
        raise ValueError("Must provide server executable")
    unittest.main()
