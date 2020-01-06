#ifndef __Renogy_hpp__
#define __Renogy_hpp__

#include "ModBus.hpp"

namespace Renogy {
static const int RATED_INPUT_VOLTAGE          = ModBus::VT_INPUT_CONTACT | 0x3000;
static const int RATED_INPUT_CURRENT          = ModBus::VT_INPUT_CONTACT | 0x3001;
static const int RATED_INPUT_POWER            = ModBus::VT_INPUT_CONTACT | 0x3002;

static const int PV_INPUT_VOLTAGE             = ModBus::VT_INPUT_CONTACT | 0x3100;
static const int PV_INPUT_CURRENT             = ModBus::VT_INPUT_CONTACT | 0x3101;
static const int PV_INPUT_POWER               = ModBus::VT_INPUT_CONTACT | 0x3102;

static const int BATTERY_VOLTAGE              = ModBus::VT_INPUT_CONTACT | 0x3104;
static const int BATTERY_CHARGING_CURRENT     = ModBus::VT_INPUT_CONTACT | 0x3105;
static const int BATTERY_STATE_OF_CHARGE      = ModBus::VT_INPUT_CONTACT | 0x311A;

static const int GENERATED_ENERGY_TODAY_L     = ModBus::VT_INPUT_CONTACT | 0x330C;
static const int GENERATED_ENERGY_TODAY_H     = ModBus::VT_INPUT_CONTACT | 0x330D;
static const int GENERATED_ENERGY_MONTH_L     = ModBus::VT_INPUT_CONTACT | 0x330E;
static const int GENERATED_ENERGY_MONTH_H     = ModBus::VT_INPUT_CONTACT | 0x330F;
static const int GENERATED_ENERGY_YEAR_L      = ModBus::VT_INPUT_CONTACT | 0x3310;
static const int GENERATED_ENERGY_YEAR_H      = ModBus::VT_INPUT_CONTACT | 0x3311;

static const int NET_BATTERY_CURRENT_L        = ModBus::VT_INPUT_CONTACT | 0x331B;
static const int NET_BATTERY_CURRENT_H        = ModBus::VT_INPUT_CONTACT | 0x331C;

static const int RTC_MINUTE_SECOND            = ModBus::VT_INPUT_REGISTER | 0x9013;
static const int RTC_HOUR_DAY                 = ModBus::VT_INPUT_REGISTER | 0x9014;
static const int RTC_MONTH_YEAR               = ModBus::VT_INPUT_REGISTER | 0x9015;

}

#endif

