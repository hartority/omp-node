#pragma once
#include "sdk.hpp"
#include "v8.h"
#include "../utils.hpp"

void WrapPlayer(IPlayer *player, v8::Local<v8::Context> context);