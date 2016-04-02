#include <string>

namespace domain_master {

    class context;
    class form_observer;

    class master {
    public:

        static const std::vector<std::string>& active_domain_names();

        // 
        void initialize();

        context& get_domain_with_name(const std::string& name);// or create if none
        context& get_default_domain();
        std::vector<std::reference_wrapper<context>>& active_domains();

        form_observer& get_form_observer();


        static master& instance();

        void clear_state();
        void read_from_stream(std::istream&);
        void write_to_stream(std::ostream&);

        // save from stream / load from stream
        // drop (or not save?) loaded contexts if no appropriate config files found?
    };


}