#ifndef KP_FIFO_PIPE_H
#define KP_FIFO_PIPE_H
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>

class FifoPipe {
public:
    FifoPipe(const std::string& get_pipe, const std::string& put_pipe);
    void put_message(const std::string& message);
    std::string get_message();
    char* get_pipe_name;
    char* put_pipe_name;
    ~FifoPipe();
private:
    static bool check_pipe(const char* pipe_name);
};

#endif //KP_FIFO_PIPE_H
