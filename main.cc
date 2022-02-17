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

void printthis(std::string temp)
{
    std::cout << temp << std::endl;
}
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

    std::vector<Command> commands{Command()};

    while (s >> token)
    {
        Command &cmd = commands.back();
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
                std::cerr << "invalid ocmmand.";
                return;
            }
            commands.push_back(Command());
        }
        else
        {
            cmd.command_tokens.push_back(token);
        }
    }

    if (commands.size() == 0)
    {
        std::cerr << "invalid command." << std::endl;

        return;
    }

    // for (int i = 0; i < commands.size(); i++)
    // {
    //     for (auto k = commands.at(i).command_tokens.begin(); k != commands.at(i).command_tokens.end(); k++)
    //     {
    //         char *s = (*k).data();
    //         std::cout << s << " ";
    //     };
    //     std::cout << std::endl;
    // };

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
            // printthis("DUPING SHIT");
            c.write_to = pipe_fd[1];
            commands.at(i + 1).read_from = pipe_fd[0];
        }

        //pipe every other command
        c.pid = fork();

        if (c.pid < 0)
        {
            std::cerr << "Forking Error" << std::endl;
            return;
        }
        if (c.pid == 0)
        {
            if (c.write_to > 0)
            {
                // std::cerr << "WRITING " << c.write_to << std::endl;
                close(pipe_fd[0]);
                int k = dup2(c.write_to, STDOUT_FILENO);
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
                close(c.read_from);
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
            if (c.read_from > 0)
            {
                close(c.read_from);
            }
            if (c.write_to > 0)
            {
                close(c.write_to);
            }
            // close(pipe_fd[0]);
            // close(pipe_fd[1]);
            // std::cerr << "PID inside:" << c.command_tokens.at(0) << " " << c.pid << std::endl;

            // int status;
            // // std::cout << "PID outside:" << c.command_tokens.at(0) << " " << c.pid << std::endl;

            // waitpid(c.pid, &status, 0);

            // if (c.command_tokens.size() > 0)
            // {
            //     std::cout << c.command_tokens.at(0) << " exit status: " << WEXITSTATUS(status) << "."
            //               << std::endl;
            // }
        }
    }

    for (int i = 0; i < vec_size; i++)
    {
        int status;

        Command c = commands.at(i);
        // std::cerr << "PID outside:" << c.command_tokens.at(0) << " " << c.pid << std::endl;

        waitpid(c.pid, &status, 0);
        std::cout << c.command_tokens.at(0) << " exit status: " << WEXITSTATUS(status) << "." << std::endl;
    }
    // for (Command c : commands)
    // {
    //     int status;
    //     std::cerr << "PID outside:" << c.command_tokens.at(0) << " " << c.pid << std::endl;

    //     waitpid(c.pid, &status, 0);
    //     std::cout << c.command_tokens.at(0) << " exit status: " << WEXITSTATUS(status) << "." << std::endl;
    //     //               << std::endl;

    //     // if (c.command_tokens.size() > 0)
    //     // {
    //     //     std::cout << c.command_tokens.at(0) << " exit status: " << WEXITSTATUS(status) << "."
    //     //               << std::endl;
    //     // }
    // }
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
