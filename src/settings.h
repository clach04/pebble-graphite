// Copyright 2016 Stefan Heule
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//        http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef GRAPHITE_SETTINGS_H
#define GRAPHITE_SETTINGS_H

#include "graphite.h"

int8_t get_current_tz_idx(TZData* data);
void update_weather(bool force);
void inbox_received_handler(DictionaryIterator *iter, void *context);
void read_config_all();
void subscribe_tick(bool also_unsubscribe);
void subscribe_tap();

#endif //GRAPHITE_SETTINGS_H
