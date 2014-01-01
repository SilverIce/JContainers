#pragma once

// Класс, содержащий мета-информацию определенного типа в виде списка.
// Может содержать, например, списки укаазателей на функции
// Предполагается, что класс будет создаваться только в статической памяти
// Class that contains meta-info
// Meta-info gets gathered during dynamic initialization and gets pushed into list
// It's assumed that meta class instance will reside in static memory only!
template<class T> class meta
{
    public: meta * next;    // temporary publicly accessible
    public: const T info;

    public: struct list {
        meta * first, * last;
        int count;
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
            l.last->next = this;
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

