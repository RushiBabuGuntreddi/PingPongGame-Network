/**
 * CS3205 - Assignment 2 (Holi'25)
 * Two-player network pingpong game using TCP sockets
 **/

 #include <ncurses.h>
 #include <pthread.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <string.h>
 #include <arpa/inet.h>
 #include <sys/socket.h>
 #include <errno.h>       // Added for errno
 #include <fcntl.h>       // Added for fcntl
 
 #define WIDTH 80
 #define HEIGHT 30
 #define OFFSETX 10
 #define OFFSETY 5
 #define PORT_DEFAULT 8080
 #define PADDLE_WIDTH 10
 
 typedef struct {
     int x, y;
     int dx, dy;
 } Ball;
 
 typedef struct {
     int x;
     int width;
 } Paddle;
 
 typedef struct {
     Ball ball;
     Paddle paddleA;  // Server paddle (bottom)
     Paddle paddleB;  // Client paddle (top)
     int scoreA, scoreB;
 } GameState;
 
 GameState game_state;
 pthread_mutex_t game_mutex = PTHREAD_MUTEX_INITIALIZER;
 int game_running = 1;
 int sockfd = -1;
 int is_server = 0;
 struct sockaddr_in serv_addr;
 
 void init();
 void end_game();
 void draw(WINDOW *win);
 void *move_ball(void *args);
 void *network_thread(void *args);
 void update_paddle(int ch);
 void reset_ball(int scored_by);
 int setup_server(int port);
 int setup_client(char *server_ip);
 
 int main(int argc, char *argv[]) {
     if (argc < 3) {
         printf("Usage: %s server PORT\n       %s client <serverip>\n", argv[0], argv[0]);
         return 1;
     }
 
     pthread_mutex_lock(&game_mutex);
     game_state = (GameState){
         .ball = {WIDTH/2, HEIGHT/2, 1, 1},
         .paddleA = {WIDTH/2 - PADDLE_WIDTH/2, PADDLE_WIDTH},
         .paddleB = {WIDTH/2 - PADDLE_WIDTH/2, PADDLE_WIDTH},
         .scoreA = 0,
         .scoreB = 0
     };
     pthread_mutex_unlock(&game_mutex);
 
     if (strcmp(argv[1], "server") == 0) {
         is_server = 1;
         int port = (argc >= 3) ? atoi(argv[2]) : PORT_DEFAULT;
         sockfd = setup_server(port);
     } else if (strcmp(argv[1], "client") == 0) {
         sockfd = setup_client(argv[2]);
     } else {
         printf("Invalid mode. Use 'server' or 'client'\n");
         return 1;
     }
     if (sockfd < 0) return 1;
 
     init();
 
     pthread_t ball_thread, net_thread;
     pthread_create(&ball_thread, NULL, move_ball, NULL);
     pthread_create(&net_thread, NULL, network_thread, NULL);
 
     while (game_running) {
         int ch = getch();
         if (ch == 'q') {
             game_running = 0;
             break;
         }
         update_paddle(ch);
         draw(stdscr);
     }
 
     pthread_join(ball_thread, NULL);
     pthread_join(net_thread, NULL);
     close(sockfd);
     end_game();
     return 0;
 }
 
 int setup_server(int port) {
     int server_fd, new_socket;
     struct sockaddr_in address;
     int opt = 1;
 
     if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) return -1;
     setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
     
     address.sin_family = AF_INET;
     address.sin_addr.s_addr = INADDR_ANY;
     address.sin_port = htons(port);
 
     if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
         perror("bind failed");
         close(server_fd);
         return -1;
     }
     if (listen(server_fd, 3) < 0) {
         perror("listen failed");
         close(server_fd);
         return -1;
     }
     int addrlen = sizeof(address);
     if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
         perror("accept failed");
         close(server_fd);
         return -1;
     }
     
     // Make socket non-blocking
     int flags = fcntl(new_socket, F_GETFL, 0);
     fcntl(new_socket, F_SETFL, flags | O_NONBLOCK);
     
     close(server_fd);
     return new_socket;
 }
 
 int setup_client(char *server_ip) {
     int sock;
     if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) return -1;
 
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_port = htons(PORT_DEFAULT);
     if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) return -1;
     if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) return -1;
     
     // Make socket non-blocking
     int flags = fcntl(sock, F_GETFL, 0);
     fcntl(sock, F_SETFL, flags | O_NONBLOCK);
     
     return sock;
 }
 
 void init() {
     initscr();
     start_color();
     init_pair(1, COLOR_BLUE, COLOR_WHITE);
     init_pair(2, COLOR_YELLOW, COLOR_YELLOW);
     timeout(10);
     keypad(stdscr, TRUE);
     curs_set(FALSE);
     noecho();
 }
 
 void end_game() {
     endwin();
 }
 
 void draw(WINDOW *win) {
     pthread_mutex_lock(&game_mutex);
     clear();
     attron(COLOR_PAIR(1));
     for (int i = OFFSETX; i <= OFFSETX + WIDTH; i++) {
         mvprintw(OFFSETY-1, i, " ");
     }
     mvprintw(OFFSETY-1, OFFSETX + 3, "CS3205 NetPong");
     mvprintw(OFFSETY-1, OFFSETX + WIDTH-25, "Player A: %d, Player B: %d", game_state.scoreA, game_state.scoreB);
     
     for (int i = OFFSETY; i < OFFSETY + HEIGHT; i++) {
         mvprintw(i, OFFSETX, "  ");
         mvprintw(i, OFFSETX + WIDTH - 1, "  ");
     }
     for (int i = OFFSETX; i < OFFSETX + WIDTH; i++) {
         mvprintw(OFFSETY, i, " ");
         mvprintw(OFFSETY + HEIGHT - 1, i, " ");
     }
     attroff(COLOR_PAIR(1));
     
     mvprintw(OFFSETY + game_state.ball.y, OFFSETX + game_state.ball.x, "o");
     
     attron(COLOR_PAIR(2));
     for (int i = 0; i < game_state.paddleA.width; i++) {
         mvprintw(OFFSETY + HEIGHT - 4, OFFSETX + game_state.paddleA.x + i, " ");
     }
     for (int i = 0; i < game_state.paddleB.width; i++) {
         mvprintw(OFFSETY + 1, OFFSETX + game_state.paddleB.x + i, " ");
     }
     attroff(COLOR_PAIR(2));
     
     refresh();
     pthread_mutex_unlock(&game_mutex);
 }
 
 void *move_ball(void *args) {
     while (game_running) {
         pthread_mutex_lock(&game_mutex);
         if (is_server) {
             game_state.ball.x += game_state.ball.dx;
             game_state.ball.y += game_state.ball.dy;
 
             if (game_state.ball.y <= 1) {
                 if (game_state.ball.x < game_state.paddleB.x || 
                     game_state.ball.x >= game_state.paddleB.x + game_state.paddleB.width) {
                     reset_ball(0);
                 } else {
                     game_state.ball.dy = -game_state.ball.dy;
                 }
             }
             if (game_state.ball.x <= 2 || game_state.ball.x >= WIDTH - 2) {
                 game_state.ball.dx = -game_state.ball.dx;
             }
             if (game_state.ball.y >= HEIGHT - 5) {
                 if (game_state.ball.x < game_state.paddleA.x || 
                     game_state.ball.x >= game_state.paddleA.x + game_state.paddleA.width) {
                     reset_ball(1);
                 } else {
                     game_state.ball.dy = -game_state.ball.dy;
                 }
             }
         }
         pthread_mutex_unlock(&game_mutex);
         usleep(80000);
     }
     return NULL;
 }
 
 void *network_thread(void *args) {
     char buffer[sizeof(GameState)];
     Paddle local_paddle;
     
     while (game_running) {
         pthread_mutex_lock(&game_mutex);
         if (is_server) {
             if (send(sockfd, &game_state, sizeof(GameState), 0) < 0) {
                 perror("Server send failed");
                 game_running = 0;
             } else {
                 ssize_t received = recv(sockfd, buffer, sizeof(Paddle), 0);
                 if (received == sizeof(Paddle)) {
                     memcpy(&game_state.paddleB, buffer, sizeof(Paddle));
                 } else if (received < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                     perror("Server receive failed");
                     game_running = 0;
                 }
             }
         } else {
             local_paddle = game_state.paddleB;
             if (send(sockfd, &local_paddle, sizeof(Paddle), 0) < 0) {
                 perror("Client send failed");
                 game_running = 0;
             } else {
                 ssize_t received = recv(sockfd, buffer, sizeof(GameState), 0);
                 if (received == sizeof(GameState)) {
                     memcpy(&game_state, buffer, sizeof(GameState));
                     game_state.paddleB = local_paddle;
                 } else if (received < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                     perror("Client receive failed");
                     game_running = 0;
                 }
             }
         }
         pthread_mutex_unlock(&game_mutex);
         usleep(80000);
     }
     return NULL;
 }
 
 void update_paddle(int ch) {
     pthread_mutex_lock(&game_mutex);
     Paddle *my_paddle = is_server ? &game_state.paddleA : &game_state.paddleB;
     if (ch == KEY_LEFT && my_paddle->x > 2) {
         my_paddle->x--;
     }
     if (ch == KEY_RIGHT && my_paddle->x < WIDTH - my_paddle->width - 2) {
         my_paddle->x++;
     }
     pthread_mutex_unlock(&game_mutex);
 }
 
 void reset_ball(int scored_by) {
     game_state.ball.x = WIDTH/2;
     game_state.ball.y = HEIGHT/2;
     game_state.ball.dx = 1;
     game_state.ball.dy = is_server ? 1 : -1;
     if (scored_by) {
         game_state.scoreB++;
     } else {
         game_state.scoreA++;
     }
    //  if (game_state.scoreA >= 10 || game_state.scoreB >= 10) {
    //      game_running = 0;
    //  }
 }