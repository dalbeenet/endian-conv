#ifndef ENDIAN_CONV_H
#define ENDIAN_CONV_H
#include <stdint.h>
#include <fstream>
#include <deque>

enum class conv_unit_type : int {
    _8_bit,
    _16_bit,
    _32_bit,
    _64_bit
};

class endian_conv {
public:
    struct config_t {
        char const* in_path;
        char const* out_path;
        uint64_t iobuf_size;
        conv_unit_type type;
    };

    enum class error_code : int;

    endian_conv();
    ~endian_conv() noexcept;
    bool exec(config_t const& cfg);
    void print_config() const;
    void trace_error() const;

protected:
    bool _init(config_t const& cfg);
    bool _cleanup() noexcept;
    bool _convert();
    void _error_log(error_code ec) noexcept;
    template <typename T>
    static void _convert_block(T const* in, T* out, int64_t count);

    config_t _cfg;
    char* _buffer_in;
    char* _buffer_out;
    std::ifstream _ifs;
    std::ofstream _ofs;
    std::deque<error_code> _err_deq;
};

enum class endian_conv::error_code {
    InputFileNotExist = 1,
    InputStreamOpenError,
    OutputStreamOpenError,
    InputStreamError,
    InputOperationError,
    OutputStreamError,
    OutputOperationError,
    BadAlloc,
    FileCloseError,
};
#endif // ENDIAN_CONV_H
