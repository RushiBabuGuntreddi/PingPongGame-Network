# CS3205 - Assignment 2
### Two-Player Network Ping Pong Game using TCP Sockets  

This project implements a **two-player ping pong game** over a network using **TCP sockets** and a text-based UI with **ncurses**.  
One player acts as the **server**, and the other joins as a **client**. The game synchronizes the ball and paddle positions across both players in real-time.

---

## 🎮 Features
- Multiplayer over TCP sockets (server–client model)  
- Real-time game updates with ball and paddle movement  
- Ncurses-based interface (works directly in the terminal)  
- Paddle control using **arrow keys**  
- Quit anytime with **`q`**  

---

## ⚙️ Requirements
- GCC or any C compiler  
- `ncurses` library  
- Linux/Unix-based system  

To install `ncurses` (if not already installed):  
```bash
sudo apt-get install libncurses5-dev libncursesw5-dev
