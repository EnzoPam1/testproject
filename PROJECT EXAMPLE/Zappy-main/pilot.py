#!/bin/python3

import sys
import socket
import threading
import curses
import curses.ascii
import time
import re


class Tile:
    def __init__(self, content: list) -> None:
        self.data = {
            "food": 0,
            "linemate": 0,
            "deraumere": 0,
            "sibur": 0,
            "mendiane": 0,
            "phiras": 0,
            "thystame": 0,
            "player": 0,
            "egg": 0,
        }

        for item in content:
            if item in self.data:
                self.data[item] += 1

    def empty(self) -> bool:
        return (
            self.data["food"] == 0
            and self.data["linemate"] == 0
            and self.data["deraumere"] == 0
            and self.data["sibur"] == 0
            and self.data["mendiane"] == 0
            and self.data["phiras"] == 0
            and self.data["thystame"] == 0
            and self.data["player"] == 0
            and self.data["egg"] == 0
        )

    def __repr__(self) -> str:
        if self.empty():
            return "·"
        return "X"


class Look:
    def __init__(self, content: str) -> None:
        self.tiles: list[Tile] = []
        for tile in content[1:-1].split(","):
            self.tiles.append(Tile(tile.split(" ")))

    def get_lvl(self, lvl) -> str:
        index = (lvl - 1) ** 2
        if index < 0 or index >= len(self.tiles):
            return ""
        s = ""
        for i in range((lvl * 2) - 1):
            s +=  repr(self.tiles[index + i])
        return s

    def get_current_tile(self) -> Tile:
        if len(self.tiles) >= 1:
            return self.tiles[0]
        else:
            return Tile([])


class ManualAI:
    def __init__(self, hostname: str, port: int) -> None:
        # Curses window
        self.scr = curses.initscr()

        self.scr.keypad(True)
        self.scr.nodelay(True)
        curses.noecho()
        curses.curs_set(0)
        curses.start_color()
        curses.use_default_colors()
        curses.init_pair(1, curses.COLOR_RED, -1)
        curses.init_pair(2, curses.COLOR_GREEN, -1)
        curses.init_pair(3, curses.COLOR_YELLOW, -1)
        curses.init_pair(4, -1, curses.COLOR_WHITE)

        # AI Data
        self.team_name = "MANUAL"
        self.map_x = 0
        self.map_y = 0

        # Network Data
        self.servaddr = (hostname, port)
        self.ntw_thread = threading.Thread(target=self.task_ntw)
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.slots_avl = 0

        # Shared Data: do not access directly unless you know what you are doing
        self.lock = threading.Lock()  # Lock for accessing the shared data
        self.connected = False  # Flag variable to check if connected to the server
        self.running = True  # Flag variable to stop the network thread
        self.buffer = ""  # Buffer for the received data
        self.msg_avl = threading.Event()  # Event to notify that a message is available

        # Start the network thread
        self.ntw_thread.start()

    def task_ntw(self):
        """
        This function is the task of the network thread.
        It connects to the server and receives data from the server.
        """
        self.scr.addstr(0, 0, f"connecting", curses.color_pair(3))
        # Connecting to the server
        with self.lock:
            try:
                self.socket.connect(self.servaddr)
            except ConnectionRefusedError:
                self.scr.addstr(
                    0, 0, f"connection refused", curses.color_pair(1))
                return

            r = self.socket.recv(1024)  # Receive the welcome message
            if r != b"WELCOME\n":
                self.scr.addstr(
                    0, 0, f"invalid welcome message", curses.color_pair(1))
                #print(r)
                return

            # Sending the team name to the server
            # and receiving the map size and the number of slots available
            self.socket.send(f"{self.team_name}\n".encode())
            r = self.socket.recv(1024)
            self.slots_avl, self.map_x, self.map_y = [
                int(x) for x in r.split()]
            if self.slots_avl > 0:
                self.connected = True

        self.socket.settimeout(0.5)
        while self.running:
            try:
                data = self.socket.recv(1024)  # Receive data from the socket
                if not data:  # connection closed by the server
                    self.connected = False

                # Adding the received data to the buffer
                with self.lock:
                    self.buffer += data.decode()

                    if "\n" in self.buffer:
                        self.msg_avl.set()

            except socket.timeout:  # Timeout occurred, check the flag variable
                pass
        self.connected = False

    def get_next_message(self) -> str | None:
        """
        This function returns the next message from the buffer.
        """
        with self.lock:
            msg = self.buffer.split("\n", 1)[0]
            self.buffer = self.buffer[len(msg) + 1:]
            return msg

    def send_message(self, msg: str) -> None:
        """
        This function sends a message to the server.
        """
        self.socket.send(f"{msg}\n".encode())

    def get_nb_messages(self) -> int:
        """
        This function returns the number of messages in the buffer.
        """
        with self.lock:
            nb = len(self.buffer.split("\n")) - 1
        return nb

    def wait_for_message(self) -> str:
        """
        This function waits for a message to be available in the buffer.
        """
        self.msg_avl.wait()
        self.msg_avl.clear()
        return self.get_next_message()

    def __del__(self):
        """
        This function is the destructor of the AI class.
        It closes the network thread and the socket.
        """
        self.running = False
        self.connected = False
        if threading.current_thread() != self.ntw_thread:
            self.ntw_thread.join()
        self.socket.close()
        curses.endwin()

    def run(self) -> None:
        """
        This function is the main function of the AI.
        """
        # Check if the AI is connected to the server
        with self.lock:
            if not self.connected:
                self.scr.nodelay(False)
                self.scr.getch()
                return

        last_key = 0
        msg = ''
        look = Look("")
        broadcasting = False
        picking = False
        dropping = False
        auto = False

        inventory = {
            "food": 0,
            "linemate": 0,
            "deraumere": 0,
            "sibur": 0,
            "mendiane": 0,
            "phiras": 0,
            "thystame": 0
        }

        while self.connected:
            current_tile = look.get_current_tile()
            c = self.scr.getch()
            if c != -1:
                last_key = c

            self.scr.clear()

            self.scr.addstr(
                0, 0, f"host: {self.servaddr[0]}:{self.servaddr[1]}")
            self.scr.addstr(1, 0, f"status:")
            if self.connected:
                self.scr.addstr(1, 8, f"connected", curses.color_pair(2))
            else:
                self.scr.addstr(1, 8, f"disconnected", curses.color_pair(1))
            self.scr.addstr(2, 0, f"running:")
            if self.running:
                self.scr.addstr(2, 9, f"yes", curses.color_pair(2))
            else:
                self.scr.addstr(2, 9, f"no", curses.color_pair(1))
            self.scr.addstr(3, 0, f"slots available: {self.slots_avl}")
            self.scr.addstr(4, 0, f"map size: {self.map_x}x {self.map_y}y")
            self.scr.addstr(
                5, 0, f"messages in buffer: {self.get_nb_messages()}")
            self.scr.addstr(6, 0, f"auto mode:")
            if auto:
                self.scr.addstr(6, 11, f"yes", curses.color_pair(2))
            else:
                self.scr.addstr(6, 11, f"no", curses.color_pair(1))
            self.scr.addstr(curses.LINES - 5, 0,
                            str(self.buffer.split('\n')[:-1])[1:-1].center(curses.COLS, ' '))
            self.scr.addstr(9, 10, "Inventory")
            self.scr.addstr(9, 20, "At reach")
            self.scr.addstr(10, 0, "food")
            self.scr.addstr(10, 10, str(inventory["food"]))
            self.scr.addstr(10, 20, str(current_tile.data["food"]))
            self.scr.addstr(11, 0, "linemate")
            self.scr.addstr(11, 10, str(inventory["linemate"]))
            self.scr.addstr(11, 20, str(current_tile.data["linemate"]))
            self.scr.addstr(12, 0, "deraumere")
            self.scr.addstr(12, 10, str(inventory["deraumere"]))
            self.scr.addstr(12, 20, str(current_tile.data["deraumere"]))
            self.scr.addstr(13, 0, "sibur")
            self.scr.addstr(13, 10, str(inventory["sibur"]))
            self.scr.addstr(13, 20, str(current_tile.data["sibur"]))
            self.scr.addstr(14, 0, "mendiane")
            self.scr.addstr(14, 10, str(inventory["mendiane"]))
            self.scr.addstr(14, 20, str(current_tile.data["mendiane"]))
            self.scr.addstr(15, 0, "phiras")
            self.scr.addstr(15, 10, str(inventory["phiras"]))
            self.scr.addstr(15, 20, str(current_tile.data["phiras"]))
            self.scr.addstr(16, 0, "thystame")
            self.scr.addstr(16, 10, str(inventory["thystame"]))
            self.scr.addstr(16, 20, str(current_tile.data["thystame"]))
            self.scr.addstr(17, 0, "player")
            self.scr.addstr(17, 20, str(current_tile.data["player"]))
            self.scr.addstr(18, 0, "egg")
            self.scr.addstr(18, 20, str(current_tile.data["egg"]))

            self.scr.addstr(9  - 5, curses.COLS - 27, "Commands :")
            self.scr.addstr(10 - 5, curses.COLS - 27, "<ENTER>  Broadcast")
            self.scr.addstr(11 - 5, curses.COLS - 27, "<P>      Pick")
            self.scr.addstr(12 - 5, curses.COLS - 27, "<D>/<S>  Set")
            self.scr.addstr(13 - 5, curses.COLS - 27, "<!>      Incantation")
            self.scr.addstr(14 - 5, curses.COLS - 27, "<F>      Fork")
            self.scr.addstr(15 - 5, curses.COLS - 27, "<E>      Eject")
            self.scr.addstr(16 - 5, curses.COLS - 27, "<SPACE>  Look")
            self.scr.addstr(17 - 5, curses.COLS - 27, "<C>      Connect_nbr")
            self.scr.addstr(18 - 5, curses.COLS - 27, "<ESC>    Quit")
            self.scr.addstr(19 - 5, curses.COLS - 27, "<A>      Auto mode")
            self.scr.addstr(20 - 5, curses.COLS - 27, "<+>      Consume response")
            self.scr.addstr(21 - 5, curses.COLS - 27, "<I>      Inventory")
            self.scr.addstr(22 - 5, curses.COLS - 27, "<UP>     Move forward")
            self.scr.addstr(23 - 5, curses.COLS - 27, "<RIGHT>  Turn right")
            self.scr.addstr(24 - 5, curses.COLS - 27, "<LEFT>   Turn left")

            center_y = curses.LINES // 2
            center_x = curses.COLS // 2
            if center_y <= 18:
                center_y = 19
            self.scr.addstr(center_y + 2, 0, ('"' + msg +
                            '"').center(curses.COLS, ' '))

            self.scr.addstr(0, center_x - 15, "BROADCASTING")
            self.scr.addstr(0, center_x, "PICKING")
            self.scr.addstr(0, center_x + 9, "DROPPING")

            self.scr.addstr(center_y - 5, center_x - 1, look.get_lvl(1))
            self.scr.addstr(center_y - 6, center_x - 2, look.get_lvl(2))
            self.scr.addstr(center_y - 7, center_x - 3, look.get_lvl(3))
            self.scr.addstr(center_y - 8, center_x - 4, look.get_lvl(4))
            self.scr.addstr(center_y - 9, center_x - 5, look.get_lvl(5))
            self.scr.addstr(center_y - 10, center_x - 6, look.get_lvl(6))
            self.scr.addstr(center_y - 11, center_x - 7, look.get_lvl(7))
            self.scr.addstr(center_y - 2, center_x - 8, look.get_lvl(8))

            if broadcasting:
                self.scr.addstr(0, center_x - 15, "BROADCASTING", curses.color_pair(4))
            if picking:
                self.scr.addstr(0, center_x, "PICKING", curses.color_pair(4))
            if dropping:
                self.scr.addstr(0, center_x + 9, "DROPPING", curses.color_pair(4))



            if broadcasting or picking or dropping:
                if c == curses.ascii.NL:
                    if broadcasting:
                        self.send_message(f"Broadcast {msg}")
                        broadcasting = False
                    elif picking:
                        self.send_message(f"Take {msg}")
                        picking = False
                    elif dropping:
                        self.send_message(f"Set {msg}")
                        dropping = False
                    msg = ''
                elif c == curses.ascii.ESC:
                    broadcasting = False
                    picking = False
                    dropping = False
                    msg = ''
                elif c == curses.KEY_BACKSPACE:
                    msg = msg[:-1]
                elif c != -1:
                    msg += chr(c)
            else:
                if c == curses.ascii.ESC:
                    self.running = False
                    self.scr.addstr(18 - 5, curses.COLS - 27, "<ESC>    Quit", curses.color_pair(4))
                elif c == curses.KEY_UP:
                    self.send_message("Forward")
                    self.scr.addstr(22 - 5, curses.COLS - 27, "<UP>     Move forward", curses.color_pair(4))
                elif c == curses.KEY_RIGHT:
                    self.send_message("Right")
                    self.scr.addstr(23 - 5, curses.COLS - 27, "<RIGHT>  Turn right", curses.color_pair(4))
                elif c == curses.KEY_LEFT:
                    self.send_message("Left")
                    self.scr.addstr(24 - 5, curses.COLS - 27, "<LEFT>   Turn left", curses.color_pair(4))
                elif c == ord('f'):
                    self.send_message("Fork")
                    self.scr.addstr(14 - 5, curses.COLS - 27, "<F>      Fork", curses.color_pair(4))
                elif c == ord(' '):
                    self.send_message("Look")
                    self.scr.addstr(16 - 5, curses.COLS - 27, "<SPACE>  Look", curses.color_pair(4))
                elif c == curses.ascii.NL:
                    broadcasting = True
                    self.scr.addstr(10 - 5, curses.COLS - 27, "<ENTER>  Broadcast", curses.color_pair(4))
                elif c == ord('i'):
                    self.send_message("Inventory")
                    self.scr.addstr(21 - 5, curses.COLS - 27, "<I>      Inventory", curses.color_pair(4))
                elif c == ord('e'):
                    self.send_message("Eject")
                    self.scr.addstr(15 - 5, curses.COLS - 27, "<E>      Eject", curses.color_pair(4))
                elif c == ord('p'):
                    picking = True
                    self.scr.addstr(11 - 5, curses.COLS - 27, "<P>      Pick", curses.color_pair(4))
                elif c == ord('!'):
                    self.send_message("Incantation")
                    self.scr.addstr(13 - 5, curses.COLS - 27, "<!>      Incantation", curses.color_pair(4))
                elif c == ord('c'):
                    self.send_message("Connect_nbr")
                    self.scr.addstr(17 - 5, curses.COLS - 27, "<C>      Connect_nbr", curses.color_pair(4))
                elif c == ord('s'):
                    dropping = True
                    self.scr.addstr(12 - 5, curses.COLS - 27, "<S>      Set", curses.color_pair(4))
                elif c == ord('d'):
                    dropping = True
                    self.scr.addstr(12 - 5, curses.COLS - 27, "<D>      Drop", curses.color_pair(4))
                elif c == ord('a'):
                    auto = not auto
                    self.scr.addstr(19 - 5, curses.COLS - 27, "<A>      Auto", curses.color_pair(4))
                elif c == ord('+') or auto:
                    if c == ord('+'):
                        self.scr.addstr(20 - 5, curses.COLS - 27, "<+>      Consume response", curses.color_pair(4))
                    m = self.get_next_message()
                    if m and m[0] == '[' and m[-1] == ']':
                        if re.search(r"\w+\s+\d+", m):
                            for s in m[1:-1].split(","):
                                k, v = s.split()
                                inventory[k] = int(v)
                        else:
                            look = Look(m)

            if last_key == curses.ascii.ESC:
                self.scr.addstr(
                    center_y, 0, f"ESCAPE".center(curses.COLS, ' '))
            elif last_key == curses.KEY_UP:
                self.scr.addstr(center_y, 0, f"UP".center(curses.COLS, ' '))
            elif last_key == curses.KEY_RIGHT:
                self.scr.addstr(center_y, 0, f"RIGHT".center(curses.COLS, ' '))
            elif last_key == curses.KEY_LEFT:
                self.scr.addstr(center_y, 0, f"LEFT".center(curses.COLS, ' '))
            elif last_key == ord(' '):
                self.scr.addstr(center_y, 0, f"SPACE".center(curses.COLS, ' '))
            elif last_key == curses.ascii.NL:
                self.scr.addstr(center_y, 0, f"ENTER".center(curses.COLS, ' '))
            elif last_key == curses.KEY_BACKSPACE:
                self.scr.addstr(
                    center_y, 0, f"BACKSPACE".center(curses.COLS, ' '))
            else:
                if last_key < 256 and last_key > 0:
                    self.scr.addstr(
                        center_y, 0, f"{chr(last_key)}".center(curses.COLS, ' '))
                else:
                    self.scr.addstr(
                        center_y, 0, f"{(last_key)}".center(curses.COLS, ' '))
            time.sleep(0.1)

        self.running = False


if __name__ == "__main__":
    try:
        args = []
        if len(sys.argv) == 3:
            args.append(sys.argv[1])
            args.append(int(sys.argv[2]))
        else:
            raise Exception(f"Usage: ./{sys.argv[0]} server_addr server_port")

        ManualAI(*args).run()

    except Exception as e:
        #print(e)
        sys.exit(1)
