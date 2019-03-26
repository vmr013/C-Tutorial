#include "server.h"

int server(int argc, char **argv) {
    if (argc != 3) {
        printf("To have a custom port pass argument to program.\n");
        server_port = PORT;
    } else {
        server_port = strtol(argv[2], NULL, 10);
    }
    int opt = TRUE;
    char *names[MAX_CLIENTS];
    int master_socket, addrlen, new_socket, client_socket[MAX_CLIENTS],
            max_clients = MAX_CLIENTS, activity, i, valread, sd;
    int max_sd;
    struct sockaddr_in address;

    char buffer[1025]; //data buffer of 1K

    //set of socket descriptors
    fd_set readfds;

    //a message
    char *message = "Welcome to Chat! \r\n";

    //initialise all client_socket[] to 0 so not checked
    for (i = 0; i < max_clients; i++) {
        client_socket[i] = 0;
    }

    //create a master socket
    if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    //set master socket to allow multiple connections ,
    //this is just a good habit, it will work without this
    if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *) &opt,
                   sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    //type of socket created
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    //bind the socket to localhost port 8888
    if (bind(master_socket, (struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Listener on port %d \n", PORT);

    //try to specify maximum of 3 pending connections for the master socket
    if (listen(master_socket, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    //accept the incoming connection
    addrlen = sizeof(address);
    puts("Waiting for connections ...");

    while (TRUE) {
        //clear the socket set
        FD_ZERO(&readfds);

        //add master socket to set
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;

        //add child sockets to set
        for (i = 0; i < max_clients; i++) {
            //socket descriptor
            sd = client_socket[i];

            //if valid socket descriptor then add to read list
            if (sd > 0)
                FD_SET(sd, &readfds);

            //highest file descriptor number, need it for the select function
            if (sd > max_sd)
                max_sd = sd;
        }

        //wait for an activity on one of the sockets , timeout is NULL ,
        //so wait indefinitely
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR)) {
            printf("select error");
        }

        //If something happened on the master socket ,
        //then its an incoming connection
        if (FD_ISSET(master_socket, &readfds)) {
            if ((new_socket = accept(master_socket,
                                     (struct sockaddr *) &address, (socklen_t * ) & addrlen)) < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            //inform user of socket number - used in send and receive commands
            printf("New connection , socket fd is %d , ip is : %s , port : %d\n", new_socket,
                   inet_ntoa(address.sin_addr), ntohs(address.sin_port));

            //send new connection greeting message
            if (send(new_socket, message, strlen(message), 0) != strlen(message)) {
                perror("send");
            }


            puts("Welcome message sent successfully");

            //add new socket to array of sockets
            for (i = 0; i < max_clients; i++) {
                //if position is empty
                if (client_socket[i] == 0) {
                    client_socket[i] = new_socket;
                    names[i] = (char *) malloc(30);
                    strcpy(names[i], inet_ntoa(address.sin_addr));
                    char *str = (char *) malloc(6);
                    snprintf(str, 7, "%d", ntohs(address.sin_port));
                    strcat(names[i], ":");
                    strcat(names[i], str);
                    free(str);
                    printf("Adding to list of sockets as %d\n", i);

                    break;
                }
            }
        }
        //else its some IO operation on some other socket
        for (i = 0; i < max_clients; i++) {
            sd = client_socket[i];

            if (FD_ISSET(sd, &readfds)) {
                //Check if it was for closing , and also read the
                //incoming message
                if ((valread = read(sd, buffer, 1024)) == 0) {
                    //Somebody disconnected , get his details and print
                    getpeername(sd, (struct sockaddr *) &address, \
                        (socklen_t * ) & addrlen);
                    printf("Host disconnected , ip %s , port %d \n",
                           inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                    //Close the socket and mark as 0 in list for reuse
                    close(sd);
                    client_socket[i] = 0;
                    free(names[i]);
                }

                    //Echo back the message that came in
                else {
                    //set the string terminating NULL byte on the end
                    //of the data read

                    buffer[valread] = '\0';
                    char *new_buffer = (char *) malloc(1048);

                    strcat(new_buffer, names[i]);
                    strcat(new_buffer, ": ");

                    strcat(new_buffer, buffer);
                    for (int j = 0; j < max_clients; ++j)
                        if (client_socket[j] != sd)
                            send(client_socket[j], new_buffer, strlen(new_buffer), 0);
                    memset(new_buffer, 0, strlen(new_buffer));
                }
            }
        }
    }

//    return 0;
}