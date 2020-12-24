#include "endian_conv.h"

template <typename T = size_t>
constexpr T MiB(const T i) {
    return i << 20;
}

int main(int argc, char*argv[]) {

    if (argc < 5) {
        printf("usage> endian-conv <in> <out> <I/O-sizeInMiB> <RecSize>\n");
        return -1;
    }

    endian_conv::config_t cfg;
    cfg.in_path = argv[1];
    cfg.out_path = argv[2];
    cfg.iobuf_size = MiB(strtoul(argv[3], nullptr, 10));

    unsigned const record_size = strtoul(argv[4], nullptr, 10);
    switch (record_size) {
    case 1:
        cfg.type = conv_unit_type::_8_bit;
        break;
    case 2:
        cfg.type = conv_unit_type::_16_bit;
        break;
    case 4:
        cfg.type = conv_unit_type::_32_bit;
        break;
    case 8:
        cfg.type = conv_unit_type::_64_bit;
        break;
    default:
        fprintf(stderr, "Invalid argument: Record size must be 1, 2, 4 or 8.\n");
        return -1;
    }

    endian_conv prog;
    if (!prog.exec(cfg)) {
        prog.trace_error();
        return -1;
    }

    return 0;
}