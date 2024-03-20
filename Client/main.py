import ipaddress
import re
import socket
import time
from threading import Thread
from time import sleep


class Application():
    receiving = False
    def __init__(self):
        self.msg_port = None
        self.st = None

    def connect(self, ip):
        try:
            self.st = socket.socket()
            ip, port = ip.split(":")
            self.st.connect((ip, int(port)))
            msg = self.st.recv(1024)
            msg = msg.decode()
            print(msg[:msg.find('\x00')])
            print()
            return
        except:
            self.st = None
            self.msg_port = None
            raise BaseException("Cannot connect")

    def disconnect(self):
        self.st.close()
        self.st = None
        self.msg_port = None

    def receive_messages(self):
        self.st.settimeout(20)
        while Application.receiving:
            try:
                msg = self.st.recv(1024).decode()
                if msg != "-":
                    print(msg)
            except socket.timeout as e:
                # err = e.args[0]
                # if err == 'timed out':
                #     sleep(1000)
                    continue

    def send_messages(self):
        while True:
            i = input()
            if i == "exit":
                return
            self.st.send(i.encode())

    def open_chat(self):
        if self.st is None:
            raise BaseException("Not connected")
        Application.receiving = True
        t1 = Thread(target=self.receive_messages)
        t2 = Thread(target=self.send_messages)
        t2.start()
        t1.start()

        t2.join()
        Application.receiving = False
        t1.join()

    def run(self):
        while True:
            print("1.Connect\n2.Disconnect\n3.Chat\n4.Exit")
            try:
                choice = input()
                if choice == "1" or choice == "Connect":
                    print("Enter IP")
                    i = input()
                    if not re.match(r"[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}:[0-9]{1,5}", i):
                        raise BaseException("Wrong IP")
                    self.connect(i)

                if choice == "2" or choice == "Disconnect":
                    self.disconnect()
                if choice == "3" or choice == "Chat":
                    self.open_chat()
                if choice == "4" or choice == "Exit":
                    return
            except BaseException as e:
                print(e)


a = Application()
a.run()
