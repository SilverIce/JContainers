namespace collections
{
    void object_base::_registerSelf() {
        jc_assert(_id == HandleNull);
        _id = context().registry->registerObject(this);
    }

    // decreases internal ref counter - _refCount OR deletes if summ refCount is 0
    // if old refCountSumm is 1 - then release, if 0 - delete
    // true, if object deleted
    bool object_base::release_from_queue() {
        jc_assert(_refCount > 0);

        --_refCount;

        if (noOwners()) {
			// it's still possible to get an access at this point?
            context().registry->removeObject(_id);

            delete this;
            return true;
        }

        return false;
    }

    void object_base::release_counter(std::atomic_int32_t& counter) {
        // allows over-release - publicly accessible tes_counter is allowed to receive redundant release calls
        //jc_assert(counter > 0);

        if (counter > 0) {
            --counter;

            if (noOwners()) {
                _prolong_lifetime();
            }
        }
    }

    void object_base::_prolong_lifetime() {
        context().aqueue->prolong_lifetime(*this);
    }
}