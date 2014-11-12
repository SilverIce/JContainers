#pragma once

#include "object_base.h"
#include "object_context.h"
#include "tes_error_code.h"
#include "spinlock.h"

#include "collections.h"

namespace collections
{
    class map;

    class tes_context : public object_context_delegate
    {
        object_context _context;
        std::atomic<Handle> _databaseId;
        std::atomic_uint_fast16_t _lastError;
        spinlock _lazyDBLock;

    public:

        tes_context()
            : _databaseId(HandleNull)
            , _lastError(0)
        {
            _context.delegate = this;
        }

        ~tes_context() {
            shutdown();
        }

        object_context& obj_context() {
            return _context;
        }

        void setLastError(JErrorCode code) {
            _lastError = code;
        }

        JErrorCode lastError() {
            uint_fast16_t code = _lastError.exchange(0);
            return (JErrorCode)code;
        }

        static tes_context& instance() {
            static tes_context st;
            return st;
        }

        Handle databaseId() const {
            return _databaseId;
        }

        map* database();

        object_stack_ref_template<map> database_ref() {
            return database();
        }

        void setDataBase(object_base *db);

        void u_loadAdditional(boost::archive::binary_iarchive & arch) override;
        void u_saveAdditional(boost::archive::binary_oarchive & arch) override;
        void u_cleanup() override;
        void u_applyUpdates(const serialization_version saveVersion) override;

        // object_context's interface

        std::vector<object_stack_ref> filter_objects(std::function<bool(object_base& obj)> predicate) const {
            return _context.filter_objects(predicate);
        }

        template<class T>
        T * getObjectOfType(Handle hdl) {
            return getObject(hdl)->as<T>();
        }

        template<class T>
        object_stack_ref_template<T> getObjectRefOfType(Handle hdl) {
            return getObjectRef(hdl)->as<T>();
        }

        size_t aqueueSize() { return _context.aqueueSize(); }
        object_base * getObject(Handle hdl) { return _context.getObject(hdl); }
        object_stack_ref getObjectRef(Handle hdl) { return _context.getObjectRef(hdl); }
        object_base * u_getObject(Handle hdl) { return _context.u_getObject(hdl); }

        void clearState() { _context.clearState(); }
        // complete shutdown, shouldn't be used for after this
        void shutdown() { _context.shutdown(); }

        void read_from_string(const std::string & data, const serialization_version version) { _context.read_from_string(data, version); }
        void read_from_stream(std::istream & data, const serialization_version version) { _context.read_from_stream(data, version); }

        std::string write_to_string() { return _context.write_to_string(); }
        void write_to_stream(std::ostream& stream) { _context.write_to_stream(stream); }

        size_t collect_garbage() {
            return _context.collect_garbage(*database());
        }
    };

}
