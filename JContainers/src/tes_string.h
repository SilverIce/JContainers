
extern std::vector<std::string> StringUtil_wrapString(const char *csource, int charsPerLine);

namespace collections {

    class tes_string : public tes_binding::class_meta_mixin_t<tes_string> {
    public:

        tes_string() {
            metaInfo.className = "JString";
            metaInfo.comment = "various string utility methods";
        }

        static object_base * wrap(const char* source, SInt32 charsPerLine) {

            if (!source || charsPerLine <= 0) {
                return nullptr;
            }

            auto strings = StringUtil_wrapString(source, charsPerLine);

            return array::objectWithInitializer([&](array *obj) {

                for (auto& str : strings) {
                    obj->_array.push_back(Item(str));
                }
            },
                tes_context::instance());
        }
        REGISTERF2(wrap, "sourceText charactersPerLine=60",
"breaks source text onto set of lines of almost equal size.\n\
returns JArray object containing lines");

    };

    TES_META_INFO(tes_string);


#ifndef TEST_COMPILATION_DISABLED

    TEST(tes_string, test)
    {
        const char *source = STR(is a 2005 American\ndocumentary film by Steve Anderson, which argues the titular word is key
        to discussions on freedom of speech and censorship. It provides perspectives from art,
        linguistics and society. Oxford English Dictionary editor Jesse Sheidlower, journalism analyst David Shaw,
        and linguists Reinhold Aman and Geoffrey Nunberg explain the terms evolution. Comedian Billy Connolly states it can
        be understood regardless of ones background, and musician Alanis Morissette says its taboo nature gives it power.
        The film contains the last interview of author Hunter S. Thompson before his suicide. It features animated sequences by Bill Plympton.
        The documentary was first screened at the AFI Film Festival at ArcLight Hollywood. The New York Times critic A. O. Scott called
        the film a battle between advocates of morality and supporters of freedom of expression; a review by the American Film Institute said
        this freedom must extend to words that offend. Other reviewers criticized the films length and repetitiveness. Its DVD
        was released in the US and the UK and used in university cour);

        auto strings = StringUtil_wrapString(source, 40);
    }

#endif

}