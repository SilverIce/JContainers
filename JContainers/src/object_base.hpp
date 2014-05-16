//BOOST_CLASS_VERSION(collections::object_base, 1);

namespace collections
{
    void object_base::_registerSelf() {
        assert(_id == HandleNull);
        _id = _context->registry->registerObject(this);
    }

    // decreases internal ref counter - _refCount OR deletes if summ refCount is 0
    // if old refCountSumm is 1 - then release, if 0 - delete
    // true, if object deleted
    bool object_base::_deleteIfNoOwner(class autorelease_queue*) {
        bool deleteObject = false; {
            mutex_lock g(_mutex);
            deleteObject = (_refCount == 0 && _tes_refCount == 0);
        }

        if (deleteObject) {
            assert(_context);
            _context->registry->removeObject(_id);
            delete this;
        }

        return deleteObject;
    }

    object_base * object_base::autorelease() {
        this->release();
        return this;
    }

    void object_base::release() {
        bool deleteObject = false; {
            mutex_lock g(_mutex);
            if (_refCount > 0) {
                --_refCount;
                deleteObject = (_refCount == 0 && _tes_refCount == 0);
            }
        }

        if (deleteObject) {
            _addToDeleteQueue();
        }
    }

    void object_base::tes_release() {
        bool deleteObject = false; {
            mutex_lock g(_mutex);
            if (_tes_refCount > 0) {
                --_tes_refCount;
                deleteObject = (_refCount == 0 && _tes_refCount == 0);
            }
        }

        if (deleteObject) {
            _addToDeleteQueue();
        }
    }

    void object_base::_addToDeleteQueue() {
        assert(_context);
        _context->aqueue->push(_id);
    }

}