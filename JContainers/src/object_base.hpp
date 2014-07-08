namespace collections
{
    void object_base::_registerSelf() {
        context().registry->registerNewObject(*this);
    }

    Handle object_base::tes_uid() {
        if (_id == HandleNull) {
            object_lock l(this);
            if (_id == HandleNull) {
                context().registry->registerNewObjectId(*this);
                context().aqueue->prolong_lifetime(*this, 0);
            }
        }

        // it's possible that release from queue will arrive just before public id will be exposed?
        return _id;
    }

    // decreases internal ref counter - _refCount OR deletes if summ refCount is 0
    // if old refCountSumm is 1 - then release, if 0 - delete
    // true, if object deleted
    bool object_base::release_from_queue() {
        jc_assert(_refCount > 0);

        --_refCount;

        if (noOwners()) {
			// it's still possible to get an access at this point?
            context().registry->removeObject(*this);

            delete this;
            return true;
        }

        return false;
    }

    void object_base::release_counter(std::atomic_int32_t& counter) {
        // publicly accessible tes_counter is allowed to receive redundant release calls
        jc_assert(&counter == &_tes_refCount || counter > 0);

        if (counter > 0) {
            --counter;

            if (noOwners()) {
                prolong_lifetime();
            }
        }
    }

    object_base* object_base::prolong_lifetime() {
        context().aqueue->prolong_lifetime(*this, is_public() ? 0 : 10);
        return this;
    }
}