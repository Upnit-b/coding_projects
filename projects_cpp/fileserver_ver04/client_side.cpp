// Client Side Implementation

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

class Client
{
private:
    int client_socket;

public:
    // CONSTRUCTOR
    Client()
    {
        client_socket = -1;
    }

    // CONNECTION MANAGEMENT

    // creating client socket
    int create_client_socket(int server_port, const std::string &server_ip)
    {
        client_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (client_socket < 0)
        {
            std::cout << "Message from client: Unable to create client socket" << std::endl;
            return -1;
        }

        // connecting with server
        sockaddr_in client_address{};
        client_address.sin_family = AF_INET;
        inet_pton(AF_INET, "127.0.0.1", &client_address.sin_addr);
        client_address.sin_port = htons(PORT);
        if (connect(client_socket, (struct sockaddr *)&client_address, sizeof(client_address)) < 0)
        {
            std::cout << "Message from client: Unable to connect with server" << std::endl;
            return -1;
        }
        std::cout << "Message from client: Connected with server successfully" << std::endl;
        return client_socket;
    }

    // FILE HANDLING

    // send file to server(Upload)
    void send_file_client(std::string &file_name)
    {
        std::ifstream file(file_name, std::ios::binary);
        if (!file.is_open())
        {
            std::cout << "Message from client: Unable to open file for sending to server" << std::endl;
            return;
        }

        // get file size
        int file_size = std::filesystem::file_size("./" + file_name);
        std::cout << "File size for sending to server: " << file_size << std::endl;
        int file_size_sent = send(client_socket, &file_size, sizeof(file_size), 0);
        std::cout << "File size sent to the server: " << file_size_sent << std::endl;

        // send file in chunks
        std::vector<char> buffer(BUFFER_SIZE);
        int total_sent = 0;
        while (total_sent < file_size)
        {
            file.read(buffer.data(), std::min(BUFFER_SIZE, int(file_size - total_sent)));
            int bytes_read = file.gcount();

            if (bytes_read > 0)
            {
                if (send(client_socket, buffer.data(), bytes_read, 0) < 0)
                {
                    std::cout << "Message from client: Error sending file to the server" << std::endl;
                }
            }
            total_sent += bytes_read;
        }
        std::cout << "Total bytes sent to the server: " << total_sent << std::endl;
        file.close();
        std::cout << "Message from client: File sent to server for upload " << file_name << std::endl;
    }

    // receiving file from server(Download)
    void receive_file_client(std::string &file_name)
    {
        std::ofstream file(file_name, std::ios::binary);
        if (!file.is_open())
        {
            std::cout << "Message from client: Unable to open file for downloading from server" << std::endl;
            return;
        }

        // receiving file size
        int file_size = 0;
        recv(client_socket, &file_size, sizeof(file_size), 0);
        std::cout << "File size received from the server: " << file_size << std::endl;

        // receiving file in chunks
        std::vector<char> buffer(BUFFER_SIZE);
        int received_bytes = 0;
        while (received_bytes < file_size)
        {
            int bytes_to_read = std::min(BUFFER_SIZE, int(file_size - received_bytes));

            int bytes_read = 0;
            if (bytes_to_read > 0)
            {
                bytes_read = recv(client_socket, buffer.data(), bytes_to_read, 0);
            }

            if (bytes_read > 0)
            {
                file.write(buffer.data(), bytes_read);
            }
            received_bytes += bytes_read;
        }
        std::cout << "Total bytes received from the server: " << received_bytes << std::endl;
        file.close();
        std::cout << "Message from client: File received from server " << file_name << std::endl;
    }

    void send_command(std::string &command)
    {
        int command_bytes_sent = send(client_socket, command.c_str(), command.size(), 0);
        std::cout << "Command bytes sent to server: " << command_bytes_sent << std::endl;
    }

    void send_file_name(std::string &file_name)
    {
        int filename_bytes_sent = send(client_socket, file_name.c_str(), file_name.size(), 0);
        std::cout << "Filename bytes sent to server: " << filename_bytes_sent << std::endl;
    }
};

int main(void)
{
    Client client;
    client.create_client_socket(8080, "127.0.0.1");

    while (true)
    {
        std::string command;
        std::cout << "Enter the Command (Upload/Download/Quit): ";
        std::cin >> command;
        std::cout << std::endl;

        if (command == "Quit")
        {
            break;
        }
        else if (command == "Upload")
        {
            client.send_command(command);
            std::string file_name;
            std::cout << "Enter filename: ";
            std::cin >> file_name;
            client.send_file_name(file_name);
            client.send_file_client(file_name);
        }
        else if (command == "Download")
        {
            client.send_command(command);
            std::string file_name;
            std::cout << "Enter filename: ";
            std::cin >> file_name;
            client.send_file_name(file_name);
            client.receive_file_client(file_name);
        }
        else
        {
            std::cout << "Invalid command. Try again" << std::endl;
            break;
        }
    }
}