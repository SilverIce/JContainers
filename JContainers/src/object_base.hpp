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

    /*

    When we need to prolong object's lifetime?
    
    - object gets exposed, returned to Skyrim for the first time, has no owners. (owned for 10 sec)
    - object has owners (another objects or aqueue), gets exposed, returned to Skyrim for the first time. Why we need to prolong is this case?:

      - if an object will be unlinked from another object-owner, @release will be called, if RC is 0 then lifetime will be prolonged. DO NOT prolong when object gets exposed
      - if an object owned by aqueue we may now know when @_final_release will be called (if no more owners, @_final_release destroys the object immediately)

    - object gets exposed second time, then gets unlinked from another object, thus @release should prolong

    */

    Handle object_base::tes_uid() {
        if (_id == HandleNull) {
            object_lock l(this);
            if (_id == HandleNull) {
                context().registry->registerNewObjectId(*this);
                // TODO: should object's lifetime be prolonged if it already has owners?
                // no owners -> should be done for sure, as we must ensure that not-owned object will not hang forever
                // has owners -> lifetime will be auto-prolonged if RC will reach zero
                if (!_refCount) {
                    prolong_lifetime();
                }
            }
        }

        // it's possible that release from queue will arrive just before public id will be exposed?
        return _id;
    }

    // decreases internal ref counter - _refCount OR deletes if summ refCount is 0
    // if old refCountSumm is 1 - then release, if 0 - delete
    // true, if object deleted
    bool object_base::_aqueue_release() {
        //jc_assert(_refCount > 0);

        if (refCount() <= 1) {
            _aqueue_refCount = 0;
			// it's still possible that something will attepmt to access to this object now?
            _delete_self();
            return true;
        }
        else {
            --_aqueue_refCount;
        }

        return false;
    }

    void object_base::_delete_self() {
        // it's still possible that something will attepmt to access to this object now?
        context().registry->removeObject(*this);
        delete this;
    }

    void object_base::tes_release() {
        if (_tes_refCount <= 1) {
            _tes_refCount = 0;
            if (noOwners()) {
                _delete_self();
            }
        }
        else {
            context().aqueue->not_prolong_lifetime(*this);
            --_tes_refCount;
        }
    }

    void object_base::stack_release() {
        // an object can be simultaneously released in diff. threads twice (example - tes_context.setDatabase) -- assertion disabled:
        //jc_assert(_refCount > 0);

        if (_stack_refCount > 0) {
            --_stack_refCount;
            if (noOwners()) {
                prolong_lifetime();
            }
        }
    }

    void object_base::release() {
        // an object can be simultaneously released in diff. threads twice (example - tes_context.setDatabase) -- assertion disabled:
        //jc_assert(_refCount > 0);

        if (_refCount > 0) {
            --_refCount;
            if (noOwners()) {
                prolong_lifetime();
            }
        }
    }

    object_base* object_base::prolong_lifetime() {
        context().aqueue->prolong_lifetime(*this, is_public());
        return this;
    }
}