#pragma once

// Класс, содержащий мета-информацию определенного типа в виде списка.
// Может содержать, например, списки укаазателей на функции
// Предполагается, что класс будет создаваться только в статической памяти
// Class that contains meta-info
// Meta-info gets gathered during dynamic initialization and gets pushed into list
// It's assumed that meta class instance will reside in static memory only!
template<class T> class meta
{
    private: meta * const next;    // temporary publicly accessible
    public: T info;

    public: class iterator {
            meta * current;
        public:
            iterator() : current(nullptr) {}
            explicit iterator(meta *elem) : current(elem) {}

            T& operator * () const { return current->info;}

            bool operator != (const iterator& other) const { return current != other.current;}

            iterator& operator ++() {
                if (current) {
                    current = current->next;
                }
                return *this;
            }
    };

    public: struct list {
            friend class meta<T>;
            meta * first, * last;
        //public:
            int count;

            iterator begin() const { return iterator(first);}
            iterator end() const { return iterator(nullptr);}
    };

    private: static list& getList() {
        static list l = {0,0,0};
        return l;
    }

    public: static const list& getListConst() {
        return getList();
    }

    public: explicit meta(const T& info2) : info(info2), next(0) {
        list& l = getList();
        if (!l.first)
            l.first = this;
        if (l.last)
            const_cast<meta *>(l.last->next) = this;
        l.last = this;
        ++l.count;
    }

    public: static int count() {
        return getList().count;
    }

    public: template<class D> static void fillList(D& newList) {
        const list& l = getList();
        meta * first = l.first;
        while(first){
            newList.push_back(first->info);
            first = first->next;
        }
    }




};

