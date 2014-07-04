namespace collections
{
    void object_base::_registerSelf() {
        assert(_id == HandleNull);
        _id = context().registry->registerObject(this);
    }

    // decreases internal ref counter - _refCount OR deletes if summ refCount is 0
    // if old refCountSumm is 1 - then release, if 0 - delete
    // true, if object deleted
    bool object_base::_deleteIfNoOwner(class autorelease_queue*) {
        if (noOwners()) {
			// it's still possible to get an access at this point?
            context().registry->removeObject(_id);

            delete this;
            return true;
        }

        return false;
    }

    object_base * object_base::autorelease() {
        this->release();
        return this;
    }

    void object_base::release_counter(std::atomic_int32_t& counter) {
        if (counter > 0) {
            --counter;

            if (noOwners()) {
                _addToDeleteQueue();
            }
        }
    }

    void object_base::_addToDeleteQueue() {
        context().aqueue->push(_id);
    }
}