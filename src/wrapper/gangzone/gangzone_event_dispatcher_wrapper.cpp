#include <utility>

#include "gangzone_wrapper.hpp"
#include "../../converter/primitive.hpp"
#include "../../converter/types.hpp"
#include "../../converter/entity.hpp"
#include "gangzone_event_dispatcher_wrapper.hpp"

WRAP_BASIC(IGangZoneEventDispatcher)
WRAP_HANDLER_BASIC(GangZoneEventHandler, NodeJSGangZoneEventHandler)

WRAP_HANDLER(NodeJSGangZoneEventHandler, void, onPlayerEnterGangZone, 2, {
    args[0] = EntityToJS<IPlayer>(player, context);
    args[1] = EntityToJS<IGangZone>(zone, context);
}, return, return, IPlayer &player, IGangZone &zone)

WRAP_HANDLER(NodeJSGangZoneEventHandler, void, onPlayerLeaveGangZone, 2, {
    args[0] = EntityToJS<IPlayer>(player, context);
    args[1] = EntityToJS<IGangZone>(zone, context);
}, return, return, IPlayer &player, IGangZone &zone)

WRAP_BASIC_CODE(IGangZoneEventDispatcher, addEventHandler, WRAP_ADD_EVENT_HANDLER(NodeJSGangZoneEventHandler))

WRAP_BASIC_CODE(IGangZoneEventDispatcher, hasEventHandler, WRAP_HAS_EVENT_HANDLER(NodeJSGangZoneEventHandler))

WRAP_BASIC_CODE(IGangZoneEventDispatcher, removeEventHandler, WRAP_REMOVE_EVENT_HANDLER(NodeJSGangZoneEventHandler))

WRAP_BASIC_CALL_RETURN(IGangZoneEventDispatcher, count, (size_t, UIntToJS))

EventDispatcherHandleStorage *WrapGangZoneEventDispatcher(IEventDispatcher<GangZoneEventHandler> *dispatcher,
                                                          v8::Local<v8::Context> context) {
    v8::HandleScope hs(context->GetIsolate());

    auto dispatcherHandle = InterfaceToObject(dispatcher, context, WRAPPED_METHODS(IGangZoneEventDispatcher));

    return new EventDispatcherHandleStorage(context->GetIsolate(), dispatcherHandle);
}