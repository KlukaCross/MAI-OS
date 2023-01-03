#include <cstring>
#include "fifo_pipe.h"

FifoPipe::FifoPipe(const std::string& get_pipe, const std::string& put_pipe) {
    this->get_pipe_name = static_cast<char *>(std::malloc(get_pipe.size() + 1));
    std::strcpy(this->get_pipe_name, get_pipe.data());
    this->put_pipe_name = static_cast<char *>(std::malloc(put_pipe.size() + 1));
    std::strcpy(this->get_pipe_name, put_pipe.data());
    if (!check_pipe(this->get_pipe_name))
        mkfifo(this->get_pipe_name, S_IRUSR | S_IWOTH);
    if (!check_pipe(this->put_pipe_name))
        mkfifo(this->put_pipe_name, S_IWUSR | S_IROTH);
}

void FifoPipe::put_message(const std::string& message) {
    std::ofstream pipe_out;
    pipe_out.open(this->put_pipe_name, std::ofstream::out);
    pipe_out << message;
    pipe_out.flush();
    pipe_out.close();
}

std::string FifoPipe::get_message() {
    std::ifstream pipe_in;
    std::string res;
    pipe_in.open(this->get_pipe_name, std::ios::in);
    getline(pipe_in, res);
    pipe_in.close();
    return res;
}

bool FifoPipe::check_pipe(const char* pipe_name) {
    struct stat statStruct{};
    int result;

    result = stat(pipe_name, &statStruct);

    return result;
}

FifoPipe::~FifoPipe() {
    // удаляем пайпы
}
