#include <cstdlib>
#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sstream>
#include <vector>
#include <set>
#include <fstream>
#include <fcntl.h>
#include <sstream>
struct Command
{
    pid_t pid;

    //must be size at least 1
    std::vector<std::string> command_tokens;

    //Can only be followed by one word
    std::string inp_redirection;
    std::string out_redirection;
};

void print_status(int status)
{
    std::cout << "exit status: " << WEXITSTATUS(status) << "."
              << std::endl;
}

void parse_and_run_command(const std::string &command)
{
    std::string token;
    std::vector<std::string> words;
    std::vector<std::string> special_tokens;
    std::istringstream s(command);

    std::set<std::string> special_token_set = {"|",
                                               "<",
                                               ">"};

    Command cmd;

    while (s >> token)
    {
        if (token == "<")
        {
            if (s >> token && cmd.inp_redirection.empty() && special_token_set.find(token) == special_token_set.end())
            {
                cmd.inp_redirection = token;
            }
            else
            {
                std::cerr << "invalid command.";
                return;
            }
        }
        else if (token == ">")
        {
            if (s >> token && cmd.out_redirection.empty() && special_token_set.find(token) == special_token_set.end())
            {
                cmd.out_redirection = token;
            }
            else
            {
                std::cout << "invalid command.";
                return;
            }
        }
        else if (token == "|")
        {
            std::cout << "invalid command.";
            return;
        }
        else
        {
            cmd.command_tokens.push_back(token);
        }
    }

    if (cmd.command_tokens.size() == 0)
    {
        std::cout << "invalid command.";
        return;
    }
    if (cmd.command_tokens.at(0) == "exit")
    {
        exit(0);
    }
    pid_t child_pid = fork();

    if (child_pid == 0)
    {
        cmd.pid = getpid();

        if (!cmd.inp_redirection.empty())
        {
            int fd = open(cmd.inp_redirection.c_str(), O_RDONLY);
            dup2(fd, STDIN_FILENO);
            close(fd);
        }

        if (!cmd.out_redirection.empty())
        {
            int fd = open(cmd.out_redirection.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        std::vector<std::string> words = cmd.command_tokens;
        std::vector<char *> chars;
        for (auto i = words.begin(); i != words.end(); i++)
        {
            char *s = (*i).data();
            chars.push_back(s);
        }
        chars.push_back(NULL);
        execv(chars[0], &chars[0]);
        perror(("Cannot execute " + words.at(0)).c_str());
        exit(1);
    }
    else
    {
        int status;
        waitpid(child_pid, &status, 0);
        print_status(status);
    }

    if (command == "exit")
    {
        exit(0);
    }
    //std::cerr << "Not implemented.\n";
}

int main(void)
{
    std::string command;
    std::cout << "> ";
    while (std::getline(std::cin, command))
    {
        parse_and_run_command(command);
        std::cout << "> ";
    }
    return 0;
}
