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
    std::vector<std::string> command_tokens;
    std::string inp_redirection;
    std::string out_redirection;
    int write_to = -1;
    int read_from = -1;
};

void parse_and_run_command(const std::string &command)
{
    std::string token;
    std::vector<std::string> words;
    std::vector<std::string> special_tokens;
    std::istringstream s(command);

    std::set<std::string> special_token_set = {"|",
                                               "<",
                                               ">"};

    std::vector<Command> commands;
    Command cmd = Command();

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
                std::cerr << "invalid command.";
                return;
            }
        }
        else if (token == "|")
        {
            if (cmd.command_tokens.size() == 0)
            {
                std::cerr << "invalid command.";
                return;
            }
            commands.push_back(cmd);
            cmd = Command();
        }
        else
        {
            cmd.command_tokens.push_back(token);
        }
    }

    commands.push_back(cmd);
    if (commands.size() == 0)
    {
        std::cerr << "invalid command." << std::endl;

        return;
    }

    for (Command c : commands)
    {
        if (c.command_tokens.size() == 0)
        {
            std::cerr << "invalid command." << std::endl;
            return;
        }
        if (c.command_tokens.at(0) == "exit")
        {
            exit(0);
        }
    }

    int vec_size = commands.size();
    int pipe_fd[2];

    for (int i = 0; i < vec_size; i++)
    {
        Command c = commands.at(i);

        pipe(pipe_fd);

        if (i != (vec_size - 1))
        {
            c.write_to = pipe_fd[1];
            commands.at(i + 1).read_from = pipe_fd[0];
        }

        pid_t child_pid = fork();

        if (child_pid < 0)
        {
            std::cerr << "Forking Error" << std::endl;
            return;
        }
        if (child_pid == 0)
        {
            if (c.write_to > 0)
            {
                // std::cerr << "WRITING " << c.write_to << std::endl;
                int k = dup2(c.write_to, STDOUT_FILENO);
                close(pipe_fd[0]);
                close(pipe_fd[1]);

                if (k < 0)
                {
                    std::cerr << "WRITING DUP FAIL" << std::endl;
                }
            }
            if (c.read_from > 0)
            {
                // std::cerr << "READING " << c.read_from << std::endl;
                int k = dup2(c.read_from, STDIN_FILENO);
                close(pipe_fd[0]);
                if (k < 0)
                {
                    std::cerr << "READING DUP FAIL" << std::endl;
                }
            }

            if (!c.inp_redirection.empty())
            {
                int fd = open(c.inp_redirection.c_str(), O_RDONLY); //check docs for values
                if (fd == -1)
                {
                    std::cerr << "File Doesnt Exist" << std::endl;
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }

            if (!c.out_redirection.empty())
            {
                int fd = open(c.out_redirection.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
                if (fd == -1)
                {
                    std::cerr << "File Doesnt Exist" << std::endl;
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }

            std::vector<std::string> words = c.command_tokens;
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
            c.pid = child_pid;
            if (c.read_from > 0)
            {
                close(c.read_from);
            }
            if (c.write_to > 0)
            {
                close(c.write_to);
            }
        }
    }

    for (int i = 0; i < vec_size; i++)
    {
        int status;

        Command c = commands.at(i);

        waitpid(c.pid, &status, 0);
        std::cout << c.command_tokens.at(0) << " exit status: " << WEXITSTATUS(status) << "." << std::endl;
    }
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
