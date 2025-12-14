#ifndef RP2040_LORA_APRS_SYSTEM_H
#define RP2040_LORA_APRS_SYSTEM_H

#include <cstdint>
#include <ThreadController.h>
#include <DS3231.h>
#include <JsonWriter.h>
#include <kiss.h>

#include "Communication.h"
#include "Radio.h"
#include "Timer.h"
#include "config.h"
#include "Command.h"
#include "GpioPin.h"
#include "Threads/EnergyThread.h"
#include "Threads/WeatherThread.h"
#include "Threads/Watchdog/WatchdogSlaveMpptChgThread.h"
#include "Threads/Watchdog/WatchdogSlaveLoraTxThread.h"
#include "Threads/Watchdog/WatchdogMasterPinThread.h"
#include "Threads/Send/SendPositionThread.h"
#include "Threads/Send/SendStatusThread.h"
#include "Threads/Send/SendTelemetriesThread.h"

#include "settings.h"
#include "Threads/Send/MeshtasticSendAprsThread.h"
#include "Threads/Send/LinuxSendAprsThread.h"

class System {
public:
    explicit System();

    bool begin();
    void loop();

    void setClock(bool slow);
    void setTimeToInternalRtc(time_t unixtime);
    bool resetEverything();
    bool resetSettings();
    bool resetAprsReceived();
    bool saveSettings();
    bool saveAprsReceived();
    void addAprsFrameReceivedToHistory(const AprsPacketLite *packet, float snr, float rssi);
    void planReboot();
    void planDfu();
    void printSettingsAndAprsReceived();
    void printJson(bool onUsb);
    void sendToKissInterface(const uint8_t* data, size_t size);
    double getTemperatureBox();

    GpioPin* getGpio(uint8_t pin);
    GpioPin* getGpio(const char* name);

    inline bool hasError() const {
        return rtcHasError || radio.hasError() || weatherThread->hasError() || energyThread->hasError();
    }

    inline bool isRtcHasError() const {
        return rtcHasError;
    }

    Settings settings{};
    SettingsAprsCallsignHeard aprsReceived[APRS_CALLSIGNS_HEARD_NUMBER]{};
    SettingsAprsCallsignHeard *lastAprsReceived = nullptr;

    EnergyThread *energyThread{};
    WeatherThread *weatherThread{};

    WatchdogSlaveLoraTxThread *watchdogSlaveLoraTxThread{};
    SendPositionThread *sendPositionThread{};
    SendStatusThread *sendStatusThread{};
    SendTelemetriesThread *sendTelemetriesThread{};
    MeshtasticSendAprsThread *sendMeshtasticAprsThread{};
    LinuxSendAprsThread *sendLinuxAprsThread{};

    Communication communication;
    Radio radio;
    Command command;
    GpioPin gpioLed = GpioPin("HeartLED", LED_BUILTIN, OUTPUT_2MA);
    GpioPin *gpiosPin[MAX_GPIO_USED]{};

    mpptChg mpptChgCharger;

    WatchdogSlaveMpptChgThread *watchdogSlaveMpptChgThread{};

    WatchdogMasterPinThread *watchdogMeshtastic{};
    WatchdogMasterPinThread *watchdogLinux{};

    DS3231 rtc;

    SettingsGetSetFunction settingsGetSetFunctions[NB_SETTINGS] = {
        { "version", UInt16, &settings.version },
        { "internalDog", Boolean, &settings.useInternalWatchdog },
        { "slowClock", Boolean, &settings.useSlowClock },
        { "rtc.enabled", Boolean, &settings.rtc.enabled },
        { "lora.freq", Float, &settings.lora.frequency },
        { "lora.bw", UInt16, &settings.lora.bandwidth },
        { "lora.sf", UInt8, &settings.lora.spreadingFactor },
        { "lora.cr", UInt8, &settings.lora.codingRate },
        { "lora.power", UInt8, &settings.lora.outputPower },
        { "lora.txEnabled", Boolean, &settings.lora.txEnabled },
        { "lora.txDog", Boolean, &settings.lora.watchdogTxEnabled },
        { "lora.txDogTime", Float, &settings.lora.intervalTimeoutWatchdogTx },
        { "aprs.call", CharString, &settings.aprs.callsign },
        { "aprs.destination", CharString, &settings.aprs.destination },
        { "aprs.path", CharString, &settings.aprs.path },
        { "aprs.pathTelem", CharString, &settings.aprs.pathTelemetry },
        { "aprs.posComment", CharString, &settings.aprs.positionComment },
        { "aprs.status", CharString, &settings.aprs.status },
        { "aprs.symbol", Char, &settings.aprs.symbol },
        { "aprs.symbolTable", Char, &settings.aprs.symbolTable },
        { "aprs.latitude", Double, &settings.aprs.latitude },
        { "aprs.longitude", Double, &settings.aprs.longitude },
        { "aprs.altitude", UInt16, &settings.aprs.altitude },
        { "aprs.digiEnabled", Boolean, &settings.aprs.digipeaterEnabled },
        { "aprs.telemEnabled", Boolean, &settings.aprs.telemetryEnabled },
        { "aprs.telemTime", UInt64, &settings.aprs.intervalTelemetry },
        { "aprs.statusEnabled", Boolean, &settings.aprs.statusEnabled },
        { "aprs.statusTime", UInt64, &settings.aprs.intervalStatus },
        { "aprs.posEnabled", Boolean, &settings.aprs.positionWeatherEnabled },
        { "aprs.posTime", UInt64, &settings.aprs.intervalPositionWeather },
        { "aprs.telemInPos", Boolean, &settings.aprs.telemetryInPosition },
        { "aprs.telemSeq", UInt16, &settings.aprs.telemetrySequenceNumber },
        { "mpptDog.enabled", Boolean, &settings.mpptWatchdog.enabled },
        { "mpptDog.timeout", UInt8, &settings.mpptWatchdog.timeout },
        { "mpptDog.feedTime", UInt64, &settings.mpptWatchdog.intervalFeed },
        { "mpptDog.timeOff", UInt16, &settings.mpptWatchdog.timeOff },
        { "weather.enabled", Boolean, &settings.weather.enabled },
        { "weather.time", UInt64, &settings.weather.intervalCheck },
        { "weather.decodeWh65B", Boolean, &settings.weather.decodeWH65B },
        { "weather.wh65BTime", UInt64, &settings.weather.intervalWH65B },
        { "energy.type", UInt8, &settings.energy.type },
        { "energy.time", UInt64, &settings.energy.intervalCheck },
        { "energy.adcPin", UInt8, &settings.energy.adcPin },
        { "energy.inaChBat", UInt8, &settings.energy.inaChannelBattery },
        { "energy.inaChSolar", UInt8, &settings.energy.inaChannelSolar },
        { "energy.pwrOffVolt", UInt16, &settings.energy.mpptPowerOffVoltage },
        { "energy.pwrOnVolt", UInt16, &settings.energy.mpptPowerOnVoltage },
        { "energy.aprsAlert", Boolean, &settings.energy.sendAprsMessageWhenAlert },
        { "energy.callAlert", CharString, &settings.energy.callsignToSendMessageAlert },
        { "i2c.enabled", Boolean, &settings.i2c.enabled },
        { "i2c.address", UInt8, &settings.i2c.address },
        { "linux.enabled", Boolean, &settings.linux.enabled },
        { "linux.dogTime", UInt64, &settings.linux.intervalTimeoutWatchdog },
        { "linux.pin.name", CharString, &settings.linux.pin.name },
        { "linux.pin.pin", UInt8, &settings.linux.pin.pin },
        { "linux.pin.mode", UInt8, &settings.linux.pin.mode },
        { "linux.pin.inverted", Boolean, &settings.linux.pin.inverted },
        { "linux.aprsItem.enabled", Boolean, &settings.linux.aprsSendItemEnabled },
        { "linux.aprsItem.time", UInt64, &settings.linux.intervalSendItem },
        { "linux.aprsItem.name", CharString, &settings.linux.itemName },
        { "linux.aprsItem.comment", CharString, &settings.linux.itemComment },
        { "linux.aprsItem.symbol", Char, &settings.linux.symbol },
        { "linux.aprsItem.symbolTable", Char, &settings.linux.symbolTable },
        { "linux.aprsItem.latitude", Double, &settings.linux.latitude },
        { "linux.aprsItem.longitude", Double, &settings.linux.longitude },
        { "linux.aprsItem.altitude", UInt16, &settings.linux.altitude },
        { "msh.enabled", Boolean, &settings.meshtastic.enabled },
        { "msh.dogTime", UInt64, &settings.meshtastic.intervalTimeoutWatchdog },
        { "msh.pin.name", CharString, &settings.meshtastic.pin.name },
        { "msh.pin.pin", UInt8, &settings.meshtastic.pin.pin },
        { "msh.pin.mode", UInt8, &settings.meshtastic.pin.mode },
        { "msh.pin.inverted", Boolean, &settings.meshtastic.pin.inverted },
        { "msh.aprsItem.enabled", Boolean, &settings.meshtastic.aprsSendItemEnabled },
        { "msh.aprsItem.time", UInt64, &settings.meshtastic.intervalSendItem },
        { "msh.aprsItem.name", CharString, &settings.meshtastic.itemName },
        { "msh.aprsItem.comment", CharString, &settings.meshtastic.itemComment },
        { "msh.aprsItem.symbol", Char, &settings.meshtastic.symbol },
        { "msh.aprsItem.symbolTable", Char, &settings.meshtastic.symbolTable },
        { "msh.aprsItem.latitude", Double, &settings.meshtastic.latitude },
        { "msh.aprsItem.longitude", Double, &settings.meshtastic.longitude },
        { "msh.aprsItem.altitude", UInt16, &settings.meshtastic.altitude },
    };
private:
    bool rtcHasError = false;
    ThreadController threadController;
    Timer timerDfu = Timer(TIME_BEFORE_REBOOT);
    Timer timerReboot = Timer(TIME_BEFORE_REBOOT);
    Timer timerPrintJson = Timer(INTERVAL_PRINT_JSON_USB, true);
    JsonWriter serialJsonWriter = JsonWriter(&Serial);
    JsonWriter serialLinuxJsonWriter = JsonWriter(&Serial1);

    kiss_packet_t kissPacket{};

    bool loadSettings();
    bool loadAprsReceived();
    void setDefaultSettings();
    void setDefaultAprsReceived();
};

#endif //RP2040_LORA_APRS_SYSTEM_H
