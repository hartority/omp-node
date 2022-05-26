#include <utility>

#include "player_wrapper.hpp"
#include "../../logger.hpp"
#include "../../converter/primitive.hpp"
#include "../../converter/types.hpp"
#include "../../converter/player.hpp"
#include "../../converter/vehicle.hpp"
#include "player_update_event_dispatcher_wrapper.hpp"

WRAP_BASIC(IPlayerUpdateEventDispatcher)
WRAP_HANDLER_BASIC(PlayerUpdateEventHandler, NodeJSPlayerUpdateEventHandler)

WRAP_HANDLER(NodeJSPlayerUpdateEventHandler, bool, onUpdate, 2, {
    args[0] = GetHandleStorageExtension(&player)->get();
    args[1] = TimePointToJS(now, context);
}, return JSToBool(cbReturnedValue, context), IPlayer& player, TimePoint now)

WRAP_BASIC_CODE(IPlayerUpdateEventDispatcher, addEventHandler, WRAP_ADD_EVENT_HANDLER(NodeJSPlayerUpdateEventHandler))

WRAP_BASIC_CODE(IPlayerUpdateEventDispatcher, hasEventHandler, WRAP_HAS_EVENT_HANDLER(NodeJSPlayerUpdateEventHandler))

WRAP_BASIC_CODE(IPlayerUpdateEventDispatcher, removeEventHandler, WRAP_REMOVE_EVENT_HANDLER(NodeJSPlayerUpdateEventHandler))

WRAP_BASIC_CALL_RETURN(IPlayerUpdateEventDispatcher, count, (size_t, UIntToJS))

v8::Local<v8::Value> WrapPlayerUpdateEventDispatcher(IEventDispatcher<PlayerUpdateEventHandler> *dispatcher,
                                               v8::Local<v8::Context> context) {

    v8::EscapableHandleScope hs(context->GetIsolate());

    auto dispatcherHandle = InterfaceToObject(dispatcher, context, WRAPPED_METHODS(IPlayerUpdateEventDispatcher));

    return hs.Escape(dispatcherHandle);
}