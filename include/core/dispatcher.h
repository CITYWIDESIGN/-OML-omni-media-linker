#ifndef OMNI_MEDIA_LINKER_DISPATCHER_H
#define OMNI_MEDIA_LINKER_DISPATCHER_H

#include "core/mapper.h"
#include "os/actions.h"

namespace oml
{
    class Dispatcher
    {
    public:
        Dispatcher(const Mapper &mapper, const Actions &actions);

        bool register_all() const;

        void unregister_all() const;

        void run_message_loop() const;

    private:
        const Mapper &mapper_;
        const Actions &actions_;
    };
}

#endif
