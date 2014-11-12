namespace collections
{
    void object_base::_registerSelf() {
        context().registry->registerNewObject(*this);
    }

    Handle object_base::public_id() {
        if (_id == HandleNull) {
            object_lock l(this);
            if (_id == HandleNull) {
                context().registry->registerNewObjectId(*this);
            }
        }

        return _id;
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
    bool object_base::_final_release() {
        //jc_assert(_refCount > 0);

        if (refCount() <= 1) {
            _refCount = 0;
			// it's still possible that something will attepmt to access to this object now?
            _delete_self();
            return true;
        }
        else {
            --_refCount;
        }

        return false;
    }

    void object_base::_delete_self() {
        // it's still possible that something will attepmt to access to this object now?
        context().registry->removeObject(*this);
        delete this;
    }

    void object_base::release_counter(std::atomic_int32_t& counter) {

        // publicly accessible tes_counter allowed to receive redundant release calls
        // _refCounter can be simultaneously released in diff. threads twice (example - tes_context.setDatabase) -- assertion disabled
        //jc_assert(&counter == &_tes_refCount || counter > 0);

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