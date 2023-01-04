#include <cstring>
#include "fifo_pipe.h"

FifoPipe::FifoPipe(const std::string& pipe_name) {
    this->pipe_name = static_cast<char *>(std::malloc(pipe_name.size() + 1));
    std::strcpy(this->pipe_name, pipe_name.data());
    if (!exists_pipe(this->pipe_name))
        mkfifo(this->pipe_name, S_IRWXU | S_IRWXG | S_IRWXO);
}

void FifoPipePut::put_message(const std::string& message) {
    std::ofstream pipe_out;
    pipe_out.open(this->pipe_name, std::ofstream::out);
    pipe_out << message;
    pipe_out.flush();
    pipe_out.close();
}

FifoPipePut::FifoPipePut(const std::string &pipe_name) : FifoPipe(pipe_name) {}

std::string FifoPipeGet::get_message() {
    std::ifstream pipe_in;
    std::string res;
    pipe_in.open(this->pipe_name, std::ios::in);
    getline(pipe_in, res);
    pipe_in.close();
    return res;
}

FifoPipeGet::FifoPipeGet(const std::string &pipe_name) : FifoPipe(pipe_name) {}

bool FifoPipe::exists_pipe(const char* pipe_name) {
    struct stat statStruct{};
    int result = stat(pipe_name, &statStruct);
    return result != -1;
}
