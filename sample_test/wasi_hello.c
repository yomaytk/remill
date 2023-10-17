#include <wasi/api.h>

void print_hellowasi() {
    const char *message = "Hello, World!\n";
    __wasi_ciovec_t iov = {
        .buf = message,
        .buf_len = 13 // length of the message
    };
    size_t bytes_written = 0;
    __wasi_fd_write(1, &iov, 1, &bytes_written);
}

int main() {
    print_hellowasi();
}