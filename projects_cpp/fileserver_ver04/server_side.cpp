// Server Side Implementation

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PORT 8080
#define BUFFER_SIZE 4096

class Server
{
private:
    std::string storage_directory;
    int server_port;
    int server_socket;
    bool server_running_status;

public:
    // CONSTRUCTOR
    Server()
    {
        std::string base_dir = "./server_files";
        storage_directory = base_dir;
        if (!std::filesystem::exists(storage_directory))
        {
            std::filesystem::create_directories(storage_directory);
        }

        server_port = PORT;

        server_running_status = false;

        server_socket = -1;
    }

    // CONNECTION MANAGEMENT

    // creating server socket
    int create_server_socket(int port)
    {
        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket < 0)
        {
            std::cout << "Message from server: Unable to create server socket" << std::endl;
            return -1;
        }

        // binding server socket to port number and server address
        sockaddr_in server_address{};
        server_address.sin_family = AF_INET;
        inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr.s_addr);
        server_address.sin_port = htons(PORT);
        if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
        {
            std::cout << "Message from server: Unable to bind server socket" << std::endl;
            return -1;
        }

        // listening for incoming connection
        if (listen(server_socket, 5) < 0)
        {
            std::cout << "Message from server: Unable to listen incoming connection" << std::endl;
            return -1;
        }
        return server_socket;
    }

    // Accepting incoming connection
    int accept_connection()
    {
        int accept_socket = accept(server_socket, nullptr, nullptr);
        if (accept_socket < 0)
        {
            std::cout << "Message from server: Unable to accept connection on server" << std::endl;
            return -1;
        }
        return accept_socket;
    }

    // start server
    void start()
    {
        int server_socket = create_server_socket(server_port);
        server_running_status = true;
        std::cout << "Message from server: Server started on port " << server_port << std::endl;

        while (server_running_status)
        {
            int accept_socket = accept_connection();
            if (accept_socket > 0)
            {
                std::thread(std::bind(&Server::handle_client, this, accept_socket)).detach();
            }
        }
    }

    // FILE HANDLING

    // receiving file from client(Upload)
    void receive_file_server(int client_socket, std::string &file_name)
    {
        std::ofstream file(storage_directory + "/" + file_name, std::ios::binary);
        if (!file.is_open())
        {
            std::cout << "Message from server: Unable to open file for receiving" << std::endl;
            return;
        }

        // receive file size
        int file_size = 0;
        recv(client_socket, &file_size, sizeof(file_size), 0);
        std::cout << "File size received from client: " << file_size << std::endl;

        // receive file in chunks
        std::vector<char> buffer(BUFFER_SIZE);
        int total_received = 0;

        while (total_received < file_size)
        {
            int bytes_read = recv(client_socket, buffer.data(), std::min(BUFFER_SIZE, int(file_size - total_received)), 0);
            if (bytes_read >= 0)
            {
                file.write(buffer.data(), bytes_read);
            }
            else
            {
                break;
            }
            total_received += bytes_read;
        }
        std::cout << "Total received bytes on server: " << total_received << std::endl;
        file.close();
        // close(client_socket);
        std::cout << "Message from server: Successfully received file named " << file_name << std::endl;
    }

    // sending file to the client(Download)
    void send_file_server(int client_socket, std::string &file_name)
    {
        std::ifstream file(storage_directory + "/" + file_name, std::ios::binary);
        if (!file.is_open())
        {
            std::cout << "Message from server: Unable to open file for sending" << std::endl;
            return;
        }

        // get file size
        int file_size = std::filesystem::file_size(storage_directory + "/" + file_name);
        std::cout << "File size for sending to server: " << file_size << std::endl;

        // send file size
        int file_size_sent = send(client_socket, &file_size, sizeof(file_size), 0);
        std::cout << "File size sent to the server: " << file_size_sent << std::endl;

        // send file in chunks
        std::vector<char> buffer(BUFFER_SIZE);
        int total_sent = 0;
        while (total_sent < file_size)
        {
            int bytes_to_read = std::min(BUFFER_SIZE, int(file_size - total_sent));

            if (bytes_to_read > 0)
            {
                file.read(buffer.data(), bytes_to_read);
            }

            int bytes_read = file.gcount();

            if (bytes_read > 0)
            {
                int bytes_sent = send(client_socket, buffer.data(), bytes_read, 0);
                if (bytes_sent <= 0)
                {
                    break;
                }
                total_sent += bytes_sent;
            }
        }
        std::cout << "Total bytes sent from server: " << total_sent << std::endl;
        file.close();
        // close(client_socket);
        std::cout << "Message from server: File sent from server " << file_name << std::endl;
    }

    // CLIENT HANDLING

    // handling incoming connection
    void handle_client(int client_socket)
    {
        while (true)
        {
            // receive command
            char command_buffer[256] = {0};
            int bytes_read_command = recv(client_socket, command_buffer, sizeof(command_buffer), 0);
            std::cout << "Total received command bytes: " << bytes_read_command << std::endl;

            if (bytes_read_command <= 0)
            {
                std::cout << "Message from server: Error receiving command" << std::endl;
                close(client_socket);
                return;
            }
            command_buffer[bytes_read_command] = '\0';
            std::string command(command_buffer);
            std::cout << "The command is received by server is: " << command << std::endl;

            if (command == "Upload")
            {
                // Receive filename
                char file_name_buffer[256] = {0};
                int bytes_read_file = recv(client_socket, file_name_buffer, sizeof(file_name_buffer), 0);
                std::cout << "Total received filename bytes: " << bytes_read_file << std::endl;
                /*if (bytes_read_file <= 0)
                {
                    std::cout << "Message from server: Error receiving filename" << std::endl;
                    close(client_socket);
                    return;
                }*/
                file_name_buffer[bytes_read_file] = '\0';
                std::cout << "File name buffer: " << file_name_buffer << std::endl;
                std::string file_name(file_name_buffer);
                std::cout << "The file name received by server is: " << file_name << std::endl;

                std::cout << "Uploading file: " << file_name << std::endl;
                receive_file_server(client_socket, file_name);
            }
            else if (command == "Download")
            {
                // Receive filename
                char file_name_buffer[256] = {0};
                int bytes_read_file = recv(client_socket, file_name_buffer, sizeof(file_name_buffer), 0);
                std::cout << "Total received filename bytes: " << bytes_read_file << std::endl;
                /*if (bytes_read_file <= 0)
                {
                    std::cout << "Message from server: Error receiving filename" << std::endl;
                    close(client_socket);
                    return;
                }*/
                file_name_buffer[bytes_read_file] = '\0';
                std::cout << "File name buffer: " << file_name_buffer << std::endl;
                std::string file_name(file_name_buffer);
                std::cout << "The file name received by server is: " << file_name << std::endl;

                std::cout << "Downloading file: " << file_name << std::endl;
                send_file_server(client_socket, file_name);
            }
            else if (command == "Quit")
            {
                return;
            }
            else
            {
                std::cout << "Invalid command: " << command << std::endl;
                return;
            }
        }

        // close(client_socket);
    }
};

int main(void)
{
    Server server;
    server.start();
}