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
    
    - object gets exposed, returned to Skyrim for the first time and HAS NO owners. (owned for 10 sec)
    - unlinked from another object,  RC is 0, @release should prolong

    When we should NOT prolong object's lifetime?

    - object has owners (another objects), gets exposed, returned to Skyrim for the first time. If an object will be unlinked from another object-owner,
      @release will be called, and if RC is 0 then lifetime will be prolonged

    STOP prolong if:

    - an object gets retained by a user
    - an object gets retained by another object

    */

    Handle object_base::uid() {
        if (_id == HandleNull) {
            object_lock l(this);
            if (_id == HandleNull) {
                context().registry->registerNewObjectId(*this);
                if (!_refCount) {
                    // prolong_lifetime if the object is not referenced by another objects -> should be done,
                    // as we must ensure that not-owned object will not hang forever
                    prolong_lifetime();
                }
            }
        }

        // it's possible that release from queue will happen just before public id will be exposed?
        return _id;
    }

	// AQueue is the only caller of the function. The function invoked when the object's lifetime expires.
    // Decreases _aqueue_refCount OR deletes the object if AQueue is the only owner of the object
    // Returns true, if object deleted
    bool object_base::_aqueue_release() {
        if (refCount() <= 1) {
            _aqueue_refCount = 0;
            _delete_self();
            return true;
        }
        else {
            --_aqueue_refCount;
        }

        return false;
    }

    void object_base::_delete_self() {
        // it's still possible that something will attepmt to access this object now?
        context().registry->removeObject(*this);
        delete this;
    }

    object_base* object_base::tes_retain() {
        ++_tes_refCount;
        context().aqueue->not_prolong_lifetime(*this);
        return this;
    }

    void object_base::tes_release() {
        if (_tes_refCount > 0) {
            --_tes_refCount;
            if (noOwners()) {
                // a user releases the object, no owners - I may even delete it immediately
                context().aqueue->prolong_lifetime(*this, false);
            }
        }
    }

    void object_base::stack_release() {
        if (_stack_refCount > 0) {
            --_stack_refCount;
            if (noOwners()) {
                // the object no more referenced by Lua or stack, no owners - I may even delete it immediately
                // (immediately if not exposed to Skyrim)
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
                // the object get's erased from another object, no owners - I may even delete it immediately
                // (immediately if not exposed to Skyrim)
                prolong_lifetime();
            }
        }
    }

    object_base* object_base::prolong_lifetime() {
        context().aqueue->prolong_lifetime(*this, is_public());
        return this;
    }

    object_base* object_base::zero_lifetime() {
        context().aqueue->not_prolong_lifetime(*this);
        return this;
    }
}