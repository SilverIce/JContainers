namespace util {
    namespace string_normalizer {

        using namespace std;
/*

        int normalize(char c);

        template<class Char>
        void char_out(Char origin, int c) {
            cout << (char)origin << ": " << (char)c << ' ' << (char)(c >> 8) << " : " << std::bitset<8>(char(c)) << ' ' << std::bitset<8>((char)(c >> 8)) << endl;
        }

        template<class Char, size_t N>
        void test(const Char(&chars)[N]) {
            for (Char c : chars) {
                char_out<Char>(c, normalize<Char>(c));
            }
        }*/


        enum {
            fst_range_size = ('Z' - 'A'),
            snd_range_size = ('z' - 'a'),
            free_bits = fst_range_size + snd_range_size, // 50 / 256
        };

        template<class Char>
        auto in_range(Char c) -> bool {
            return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z');
        }

        template<class Char>
        auto assert_in_range(Char c) -> Char {
            if (!in_range(c)) {
                throw std::runtime_error("totot");
            }
            return c;
        }

        template<class Char>
        auto normalize(Char c) -> uint32_t {

            if (in_range(c)) {
                // ok
                return c;
            }
            else
            {
                auto pack = [](Char n) -> uint32_t {

                    static_assert(std::is_unsigned<Char>::value, "");

                    if (n < fst_range_size) {
                        return assert_in_range('A' + n);
                    }
                    else if (n < free_bits) {
                        return assert_in_range('a' + (n - fst_range_size));
                    }
                    else
                        throw std::runtime_error("totot");
                };

                return pack(c % free_bits) | (pack(c / free_bits) << 8);
            }
        }

        template<class InputRange>
        inline auto normalize_string(InputRange& range) -> std::string {
            std::string result{};
            result.reserve(range.size() * 2);
            for (auto& chr : range) {
                auto normchar = normalize((uint8_t)chr);
                result += normchar;
                if (normchar > 0xff) {
                    result += (char)(normchar >> 8);
                }
            }
            return result;
        }
    }

    using util::string_normalizer::normalize_string;
}
