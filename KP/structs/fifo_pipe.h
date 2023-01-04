#ifndef KP_FIFO_PIPE_H
#define KP_FIFO_PIPE_H
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>

class FifoPipe {
public:
    explicit FifoPipe(const std::string& pipe_name);
    char* pipe_name;
protected:
    static bool exists_pipe(const char* pipe_name);
};

class FifoPipeGet : public FifoPipe {
public:
    explicit FifoPipeGet(const std::string& pipe_name);
    std::string get_message();
};

class FifoPipePut : public FifoPipe {
public:
    explicit FifoPipePut(const std::string& pipe_name);
    void put_message(const std::string& message);
};

#endif //KP_FIFO_PIPE_H
