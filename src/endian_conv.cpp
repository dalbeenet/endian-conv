#include "endian_conv.h"
#include <indicators/cursor_control.hpp>
#include <indicators/progress_bar.hpp>
#include <boost/filesystem.hpp>
#include <boost/endian/conversion.hpp>
#include <assert.h>

#define _CRT_SECURE_NO_WARNINGS
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

endian_conv::endian_conv() {
    memset(&_cfg, 0, sizeof(config_t));
    _buffer_in = nullptr;
    _buffer_out = nullptr;
    // ifs
    // ofs
}

endian_conv::~endian_conv() noexcept {
    _cleanup();
}

bool endian_conv::exec(config_t const& cfg) {
    namespace fs = boost::filesystem;
    assert(cfg.iobuf_size > 0);
    if (!fs::exists(cfg.in_path)) {
        _error_log(error_code::InputFileNotExist);
        return false;
    }

    if (!_init(cfg) || !_convert())
        return false;

    return _cleanup();
}

void endian_conv::print_config() const {
    printf("input file path: %s\n", _cfg.in_path);
    printf("output file path: %s\n", _cfg.out_path);
    printf("buffer size: %" PRIu64 "\n", _cfg.iobuf_size);
}

void endian_conv::trace_error() const {
    printf("trace> ");
    for (auto const& it : _err_deq) {
        printf("%d => ", it);
    }
    printf("<end>\n");
}

bool endian_conv::_init(config_t const& cfg) {
    _cfg = cfg;

    // open file streams
    _ifs.open(cfg.in_path, std::ios::in | std::ios::binary);
    if (!_ifs.is_open()) {
        _error_log(error_code::InputStreamOpenError);
        goto lb_err;
    }
    _ofs.open(cfg.out_path, std::ios::out | std::ios::binary);
    if (!_ofs.is_open()) {
        _error_log(error_code::OutputStreamOpenError);
        goto lb_err;
    }

    // allocate I/O buffers
    assert(_buffer_in == nullptr);
    _buffer_in = static_cast<char*>(malloc(cfg.iobuf_size));
    if (_buffer_in == nullptr) {
        _error_log(error_code::BadAlloc);
        goto lb_err;
    }
    assert(_buffer_out == nullptr);
    _buffer_out = static_cast<char*>(malloc(cfg.iobuf_size));
    if (_buffer_out == nullptr) {
        _error_log(error_code::BadAlloc);
        goto lb_err;
    }

    memset(_buffer_in, 0, cfg.iobuf_size);
    memset(_buffer_out, 0, cfg.iobuf_size);

    return true;

lb_err:
    _cleanup();
    return false;
}

bool endian_conv::_cleanup() noexcept {
    try {
        _ifs.close();
        _ofs.close();
        return true;
    }
    catch (std::exception const& ex) {
        fprintf(stderr, "Exception detected: %s in file %s:%d\n", ex.what(), __FILE__, __LINE__);
        _error_log(error_code::FileCloseError);
    }
    catch (...) {
        fprintf(stderr, "Exception detected: <UNKNOWN> in file %s:%d\n", __FILE__, __LINE__);
        _error_log(error_code::FileCloseError);
    }
    free(_buffer_in); _buffer_in = nullptr;
    free(_buffer_out); _buffer_out = nullptr;

    return false;
}

bool endian_conv::_convert() {
    namespace fs = boost::filesystem;
    using namespace indicators;
    uint64_t const full_size = fs::file_size(_cfg.in_path);
    uint64_t remained_size = full_size;
    assert(remained_size > 0);

    ProgressBar bar{
       option::BarWidth{50},
       option::Start{"["},
       option::Fill{"="},
       option::Lead{">"},
       option::Remainder{" "},
       option::End{"]"},
       option::PostfixText{"Converting..."},
       option::ForegroundColor{Color::white},
       option::FontStyles{std::vector<FontStyle>{FontStyle::bold}}
    };

    show_console_cursor(false);
    bar.set_progress(0);

    bool result = false;

    uint64_t read_unit_size = _cfg.iobuf_size;

    switch (_cfg.type) {
    case conv_unit_type::_8_bit:
        break;
    case conv_unit_type::_16_bit:
        read_unit_size -= _cfg.iobuf_size % sizeof(uint16_t);
        break;
    case conv_unit_type::_32_bit:
        read_unit_size -= _cfg.iobuf_size % sizeof(uint32_t);
        break;
    case conv_unit_type::_64_bit:
        read_unit_size -= _cfg.iobuf_size % sizeof(uint64_t);
        break;
    }

    do {
        _ifs.read(_buffer_in, read_unit_size);
        if (_ifs.rdstate() == std::ios_base::badbit) {
            _error_log(error_code::InputStreamError);
            goto lb_ret;
        }
        if (_ifs.rdstate() == std::ios_base::failbit) {
            _error_log(error_code::InputOperationError);
            goto lb_ret;
        }

        uint64_t const read_size = static_cast<uint64_t>(_ifs.gcount());

        switch (_cfg.type) {
        case conv_unit_type::_8_bit:
            assert(read_size % sizeof(uint8_t) == 0);
            _convert_block(reinterpret_cast<uint8_t const*>(_buffer_in), reinterpret_cast<uint8_t*>(_buffer_out), static_cast<int64_t>(read_size / 4));
            break;
        case conv_unit_type::_16_bit:
            assert(read_size % sizeof(uint16_t) == 0);
            _convert_block(reinterpret_cast<uint16_t const*>(_buffer_in), reinterpret_cast<uint16_t*>(_buffer_out), static_cast<int64_t>(read_size / 4));
            break;
        case conv_unit_type::_32_bit:
            assert(read_size % sizeof(uint32_t) == 0);
            _convert_block(reinterpret_cast<uint32_t const*>(_buffer_in), reinterpret_cast<uint32_t*>(_buffer_out), static_cast<int64_t>(read_size / 4));
            break;
        case conv_unit_type::_64_bit:
            assert(read_size % sizeof(uint64_t) == 0);
            _convert_block(reinterpret_cast<uint64_t const*>(_buffer_in), reinterpret_cast<uint64_t*>(_buffer_out), static_cast<int64_t>(read_size / 4));
            break;
        }
        remained_size -= read_size;

        size_t const progress = static_cast<size_t>(static_cast<double>(full_size - remained_size) / static_cast<double>(full_size) * 100.0);
        bar.set_progress(progress);

        _ofs.write(_buffer_out, read_size);
        if (_ofs.rdstate() == std::ios_base::badbit) {
            _error_log(error_code::OutputStreamError);
            goto lb_ret;
        }
        if (_ofs.rdstate() == std::ios_base::failbit) {
            _error_log(error_code::OutputOperationError);
            goto lb_ret;
        }
    } while (remained_size > 0);

    result = true;

lb_ret:
    show_console_cursor(true);
    return result;
}

void endian_conv::_error_log(error_code const ec) noexcept {
    try {
        _err_deq.push_back(ec);
    }
    catch (std::exception const& ex) {
        fprintf(stderr, "Failed to write an error log (Error code: %d); Exception detected: %s in file %s:%d\n", ec, ex.what(), __FILE__, __LINE__);
    }
    catch (...) {
        fprintf(stderr, "Failed to write an error log (Error code: %d); Exception detected: <Unknown> in file %s:%d\n", ec, __FILE__, __LINE__);
    }
}

template <typename T>
void endian_conv::_convert_block(T const* in, T* out, int64_t count) {
#pragma omp parallel for
    for (int64_t i = 0; i < count; ++i) {
        out[i] = boost::endian::endian_reverse(in[i]);
    }
}
