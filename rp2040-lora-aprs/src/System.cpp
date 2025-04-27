#include <hardware/rtc.h>
#include <LittleFS.h>

#include "ArduinoLog.h"
#include "System.h"

#include <hardware/pll.h>
#include <hardware/vreg.h>

#include "Threads/Energy/EnergyMpptChgThread.h"
#include "Threads/Energy/EnergyIna3221Thread.h"
#include "Threads/Energy/EnergyAdcThread.h"
#include "Threads/Energy/EnergyDummyThread.h"

#include "Threads/BlinkerThread.h"

#include "I2CSlave.h"
#include "utils.h"
#include "PicoSleep.h"

System::System() : communication(this), radio(this), command(this) {
    timerReboot.pause();
    timerDfu.pause();
}

bool System::begin() {
    Log.infoln(F("[SYSTEM] Starting"));

    randomSeed(analogRead(A1));

    if (watchdog_enable_caused_reboot()) {
        Log.warningln(F("[SYSTEM] Watchdog caused reboot"));
        ledBlink(3, 500);
    } else {
        ledBlink();
    }

    LittleFS.begin();
    if (!loadSettings()) {
        ledBlink(3, 2000);
    }

    setClock(settings.useSlowClock);

    rtc_init();
    Wire.begin();

    uint8_t gpioI = 0;
    gpiosPin[gpioI++] = &gpioLed;

    threadController.add(new BlinkerThread(this, &gpioLed));

    if (settings.useInternalWatchdog) {
        rp2040.wdt_begin(8300);
        Log.infoln(F("[SYSTEM] Internal watchdog enabled"));
    }

    if (settings.rtc.enabled) {
        const auto now = RTClib::now();
        if (now.year() >= 2025 && now.year() <= 2060) {
            setTimeToInternalRtc(now.unixtime());
        } else {
            setTimeToInternalRtc(0);
            Log.warningln(F("[RTC] Wrong rtc time !"));
        }
    } else {
        setTimeToInternalRtc(0);
    }

    threadController.add(&radio);

    switch (settings.energy.type) {
        case dummy:
            energyThread = new EnergyDummyThread(this);
        break;
        case mpptchg:
            energyThread = new EnergyMpptChgThread(this, new uint16_t[11] { 12700, 12500, 12420, 12320, 12200, 12060, 11900, 11750, 11580, 11310, 10500}, 11);
            break;
        case ina:
            energyThread = new EnergyIna3221Thread(this);
            break;
        case adc:
            energyThread = new EnergyAdcThread(this);
            break;
    }
    threadController.add(energyThread);

    weatherThread = new WeatherThread(this);
    threadController.add(weatherThread);

    if (settings.i2c.enabled) {
        I2CSlave::begin(this);
    }

    watchdogSlaveMpptChgThread = new WatchdogSlaveMpptChgThread(this);
    if (settings.energy.type == mpptchg) {
        threadController.add(watchdogSlaveMpptChgThread);
    }

    watchdogSlaveLoraTxThread = new WatchdogSlaveLoraTxThread(this);
    threadController.add(watchdogSlaveLoraTxThread);

    for (auto &[pin, mode, inverted, name] : settings.pins) {
        if (pin > 0) {
            gpiosPin[gpioI++] = new GpioPin(name, pin, mode, false, inverted);
        }
    }

    auto *gpioMeshtastic = new GpioPin(settings.meshtastic.pin.name, settings.meshtastic.pin.pin, settings.meshtastic.pin.mode, false, settings.meshtastic.pin.inverted);
    gpiosPin[gpioI++] = gpioMeshtastic;
    watchdogMeshtastic = new WatchdogMasterPinThread(this, PSTR("MESHTASTIC"), gpioMeshtastic, settings.meshtastic.intervalTimeoutWatchdog, settings.meshtastic.enabled);
    threadController.add(watchdogMeshtastic);

    auto *gpioLinuxBoard = new GpioPin(settings.linux.pin.name, settings.linux.pin.pin, settings.linux.pin.mode, false, settings.linux.pin.inverted);
    gpiosPin[gpioI++] = gpioLinuxBoard;
    watchdogLinux = new WatchdogMasterPinThread(this, PSTR("LINUX"), gpioLinuxBoard, settings.linux.intervalTimeoutWatchdog, settings.linux.enabled);
    threadController.add(watchdogLinux);

    sendPositionThread = new SendPositionThread(this);
    threadController.add(sendPositionThread);

    sendStatusThread = new SendStatusThread(this);
    threadController.add(sendStatusThread);

    sendTelemetriesThread = new SendTelemetriesThread(this);
    threadController.add(sendTelemetriesThread);

    sendMeshtasticAprsThread = new MeshtasticSendAprsThread(this);
    threadController.add(sendMeshtasticAprsThread);

    sendLinuxAprsThread = new LinuxSendAprsThread(this);
    threadController.add(sendLinuxAprsThread);

    for(int i = 0; i < MAX_THREADS ; i++) {
        if (const auto thread = static_cast<MyThread *>(threadController.get(i)); thread != nullptr) { // NOLINT(*-pro-type-static-cast-downcast)
            if (!thread->enabled) {
                continue;
            }

            if (!thread->begin()) {
                Log.errorln(F("[SYSTEM] Thread %s init KO"), thread->ThreadName.c_str());
                continue;
            }

            if (thread->shouldRun(millis())) {
                thread->run();
            }
        }
    }

    Log.infoln(F("[SYSTEM] Started"));

    return true;
}

void System::loop() {
    Stream *streamReceived = nullptr;

    if (Serial.available()) {
        streamReceived = &Serial;
        Log.traceln(F("Serial USB incoming"));
    } else if (Serial1.available()) {
        streamReceived = &Serial1;
        Log.traceln(F("Serial UART 0 incoming (Linux)"));

        if (watchdogLinux->enabled) {
            watchdogLinux->feed();
        }
    }
#if USE_KISS
    else if (Serial2.available()) {
        streamReceived = &Serial2;
        Log.traceln(F("Serial UART 1 incoming (KISS)"));

        if (watchdogLinux->enabled) {
            watchdogLinux->feed();
        }
    }
#endif

    if (streamReceived != nullptr) {
#if USE_KISS
        if (streamReceived == &Serial2) {
            kissPacket = kiss_new_packet(buffer, BUFFER_LENGTH);
            size_t bytesRead = 0;
            while (streamReceived->available()) {
                buffer[bytesRead++] = streamReceived->read();

                size_t bytesConsumed = kiss_decode_packet(&kissPacket, buffer, bytesRead);

                bytesRead -= bytesConsumed;

                if (kissPacket.complete_packet) {
                    Log.infoln(F("[SERIAL_KISS] Received %d of data from KISS OK"), kissPacket.data_length);
                    radio.send(kissPacket.data, kissPacket.data_length);
                }
            }
        } else
#endif
        {
            const size_t lineLength = streamReceived->readBytesUntil('\n', bufferText, BUFFER_LENGTH - 5);
            bufferText[lineLength] = '\0';

            Log.traceln(F("[SERIAL] Received %s"), bufferText);

            command.processCommand(streamReceived, bufferText);
        }

        streamReceived->flush();
    } else {
        threadController.run();
    }

    if (timerPrintJson.hasExpired()) {
        printJson(true);
        timerPrintJson.restart();
    }

    if (timerReboot.hasExpired()) {
        rp2040.reboot();
        return;
    }

    if (timerDfu.hasExpired()) {
        if (settings.energy.type == mpptchg && watchdogSlaveMpptChgThread->enabled) {
            watchdogSlaveMpptChgThread->setManagedByUser(TIME_SET_MPPT_WATCHDOG_DFU);
        }

        rp2040.rebootToBootloader();
        return;
    }

    delay(10);

    rp2040.wdt_reset();
}

void System::setTimeToInternalRtc(const time_t epoch) {
    datetime_t datetime;
    epoch_to_datetime(epoch, &datetime);
    rtc_set_datetime(&datetime);
    getDateTimeStringFromEpoch(epoch, bufferText, BUFFER_LENGTH);
    Log.infoln(F("[RTC] Set internal RTC to date %s"), bufferText);
}

bool System::loadSettings() {
    File file = LittleFS.open("/config.dat", "r");
    if (!file) {
        Log.warningln(F("[CONFIG] Fail to open settings, we create it"));

        setDefaultSettings();

        return saveSettings();
    }

    file.read(reinterpret_cast<uint8_t *>(&settings), sizeof(settings));
    file.close();

    Log.infoln(F("[CONFIG] Read correctly"));

    if (settings.version < SETTINGS_VERSION) {
        setDefaultSettings();
        setDefaultAprsReceived();
        saveSettings();
        saveAprsReceived();
    }

    printSettingsAndAprsReceived();

    return true;
}

bool System::loadAprsReceived() {
    File file = LittleFS.open("/aprs.dat", "r");
    if (!file) {
        Log.warningln(F("[CONFIG] Fail to open aprs received, we create it"));

        setDefaultAprsReceived();

        return saveAprsReceived();
    }

    file.read(reinterpret_cast<uint8_t *>(&aprsReceived), sizeof(aprsReceived));
    file.close();

    Log.infoln(F("[CONFIG] Read correctly"));

    printSettingsAndAprsReceived();

    return true;
}

bool System::saveSettings() {
    File file = LittleFS.open("/config.dat", "w");
    if (!file) {
        Log.errorln(F("[CONFIG] Fail to save settings, we use the default one"));
        printSettingsAndAprsReceived();
        return false;
    }

    file.write(reinterpret_cast<const uint8_t *>(&settings), sizeof(settings));
    file.close();

    Log.infoln(F("[CONFIG] Saved settings to FS"));

    return true;
}

bool System::saveAprsReceived() {
    File file = LittleFS.open("/aprs.dat", "w");
    if (!file) {
        Log.errorln(F("[CONFIG] Fail to save aprs received, we use the default one"));
        printSettingsAndAprsReceived();
        return false;
    }

    file.write(reinterpret_cast<const uint8_t *>(&aprsReceived), sizeof(aprsReceived));
    file.close();

    Log.infoln(F("[CONFIG] Saved aprs received to FS"));

    return true;
}

void System::addAprsFrameReceivedToHistory(const AprsPacketLite *packet, const float snr, const float rssi) {
    uint8_t frameIndex = 0;

    for (const auto &oldFrame : aprsReceived) {
        if (strcmp(oldFrame.callsign, packet->source) == 0 || strlen(oldFrame.callsign) == 0) {
            break;
        }

        frameIndex++;
    }

    if (frameIndex >= APRS_CALLSIGNS_HEARD_NUMBER) {
        Log.warningln(F("[APRS_HISTORY] Maximum reached !"));
        return;
    }

    lastAprsReceived = &aprsReceived[frameIndex];

    lastAprsReceived->time = getDateTime().unixtime();
    lastAprsReceived->snr = snr;
    lastAprsReceived->rssi = rssi;
    lastAprsReceived->count++;
    lastAprsReceived->digipeaterCount = packet->digipeaterCount;
    strcpy(lastAprsReceived->callsign, packet->source);
    strcpy(lastAprsReceived->content, packet->raw);
    strcpy(lastAprsReceived->digipeaterCallsign, packet->lastDigipeaterCallsignInPath);

    saveSettings();
}

bool System::resetEverything() {
    if (!LittleFS.format()) {
        Log.errorln(F("[CONFIG] Fail to format"));
        return false;
    }

    Log.infoln(F("[CONFIG] Formated"));

    loadSettings();

    return true;
}

bool System::resetSettings() {
    if (LittleFS.exists("/config.dat") && !LittleFS.remove("/config.dat")) {
        Log.errorln(F("[CONFIG] Fail to delete settings"));
        return false;
    }

    Log.infoln(F("[CONFIG] Settings deleted"));

    loadSettings();

    return true;
}

bool System::resetAprsReceived() {
    if (LittleFS.exists("/aprs.dat") && !LittleFS.remove("/aprs.dat")) {
        Log.errorln(F("[CONFIG] Fail to delete aprs received"));
        return false;
    }

    Log.infoln(F("[CONFIG] Aprs received deleted"));

    loadAprsReceived();

    return true;
}

void System::setDefaultSettings() {
    settings.version = SETTINGS_VERSION;
    settings.useInternalWatchdog = true;
    settings.useSlowClock = false;

    settings.lora.frequency = 433.775;
    settings.lora.bandwidth = 125;
    settings.lora.spreadingFactor = 12;
    settings.lora.codingRate = 5;
    settings.lora.outputPower = 22;
    settings.lora.txEnabled = true;
    settings.lora.boostedRxGain = true;
    settings.lora.watchdogTxEnabled = true;
    settings.lora.intervalTimeoutWatchdogTx = 7200000; // 2 hours
    strcpy_P(settings.aprs.pathTelemetry, PSTR("F4HVV-10"));

#ifdef GRAND_RATZ
    strcpy_P(settings.aprs.callsign, PSTR("F4HVV-15"));
#elifdef SAINT_JEAN
    strcpy_P(settings.aprs.callsign, PSTR("F4HVV-14"));
#endif
    strcpy_P(settings.aprs.destination, PSTR("APLV1"));
    strcpy_P(settings.aprs.path, PSTR("WIDE1-1"));
    settings.aprs.symbol = '#';
    settings.aprs.symbolTable = 'L';
#ifdef GRAND_RATZ
    settings.aprs.latitude = 45.325776;
    settings.aprs.longitude = 5.636580;
    settings.aprs.altitude = 850;
#elifdef SAINT_JEAN
    settings.aprs.latitude = 0;
    settings.aprs.longitude = 0;
    settings.aprs.altitude = 0;
#endif
    settings.aprs.positionComment[0] = '\0';
    strcpy_P(settings.aprs.status, PSTR("Digi LoRa solaire"));
    settings.aprs.positionWeatherEnabled = true;
    settings.aprs.intervalPositionWeather = 3600000; // 60 minutes
    settings.aprs.telemetryEnabled = true;
    settings.aprs.telemetrySequenceNumber = 0;
    settings.aprs.telemetryInPosition = false;
    settings.aprs.intervalTelemetry = 900000; // 15 minutes
    settings.aprs.statusEnabled = true;
    settings.aprs.intervalStatus = 86400000; // 1 day
    settings.aprs.digipeaterEnabled = true;

    settings.i2c.enabled = true;
    settings.i2c.address = 0x11;

#ifdef GRAND_RATZ
    byte i = 0;
    settings.pins[i].pin = 11;
    strcpy_P(settings.pins[i++].name, PSTR("wifi"));
    settings.pins[i].pin = 12;
    strcpy_P(settings.pins[i++].name, PSTR("npr"));
    settings.pins[i].pin = 9;
    strcpy_P(settings.pins[i++].name, PSTR("linux"));
    settings.pins[i].pin = 10;
    strcpy_P(settings.pins[i++].name, PSTR("msh"));
#endif

    settings.meshtastic.enabled = true;
    settings.meshtastic.intervalTimeoutWatchdog = 300000; // 5 minutes
    settings.meshtastic.pin.pin = 10;
    strcpy_P(settings.meshtastic.pin.name, PSTR("msh"));
    settings.meshtastic.aprsSendItemEnabled = true;
    settings.meshtastic.intervalSendItem = 3600000; // 1 hour
    strcpy_P(settings.meshtastic.itemName, PSTR("MSH"));
    settings.meshtastic.symbol = '#';
    settings.meshtastic.symbolTable = '\\';
    strcpy_P(settings.meshtastic.itemComment, PSTR("Meshtastic LongModerate 869.4625"));
#ifdef GRAND_RATZ
    settings.meshtastic.latitude = 45.325796;
    settings.meshtastic.longitude = 5.636426;
    settings.meshtastic.altitude = 850;
#elifdef SAINT_JEAN
    settings.meshtastic.latitude = 0;
    settings.meshtastic.longitude = 0;
    settings.meshtastic.altitude = 0;
#endif

#ifdef GRAND_RATZ
    settings.mpptWatchdog.enabled = true;
    settings.mpptWatchdog.timeout = 90; // 1,5 minutes
    settings.mpptWatchdog.timeOff = 10; // 10 seconds
    settings.mpptWatchdog.intervalFeed = 30000; // 30 seconds
#else
    settings.mpptWatchdog.enabled = false;
#endif

#ifdef GRAND_RATZ
    settings.energy.type = mpptchg;
    settings.energy.mpptPowerOffVoltage = 11550;
    settings.energy.mpptPowerOnVoltage = 12000;
    settings.energy.sendAprsMessageWhenAlert = true;
    strcpy_P(settings.energy.callsignToSendMessageAlert, PSTR("F4HVV-7"));
#elifdef SAINT_JEAN
    settings.energy.type = dummy;
    // settings.energy.type = ina;
    settings.energy.inaChannelBattery = INA3221_CH1;
    settings.energy.inaChannelSolar = INA3221_CH2;
#endif
    settings.energy.intervalCheck = 30000; // 30 seconds

    settings.weather.enabled = true;
    settings.weather.intervalCheck = 60000; // 60 seconds
    settings.weather.intervalWH65B = 900000; // 15 minutes

#ifdef GRAND_RATZ
    settings.weather.decodeWH65B = false;
#elif SAINT_JEAN
    settings.weather.enabled = false;
    settings.weather.decodeWH65B = true;
    settings.weather.intervalWH65B = 30000; // 30 seconds
#endif

#ifdef GRAND_RATZ
    settings.linux.enabled = false;
    settings.linux.intervalTimeoutWatchdog = 1200000; // 20 minutes
    settings.linux.pin.pin = 9;
    strcpy_P(settings.linux.pin.name, PSTR("linux"));
    settings.linux.aprsSendItemEnabled = true;
    settings.linux.intervalSendItem = 3600000; // 1 hour
    strcpy_P(settings.linux.itemName, PSTR("CAMIP"));
    settings.linux.symbol = 'I';
    settings.linux.symbolTable = '/';
    strcpy_P(settings.linux.itemComment, PSTR("f4hvv.valentin-saugnier.fr/f4hvv-15"));
    settings.linux.latitude = 45.325688;
    settings.linux.longitude = 5.636493;
    settings.linux.altitude = 850;
#else
    settings.linux.enabled = false;
    settings.linux.aprsSendItemEnabled = false;
#endif

#ifdef GRAND_RATZ
    settings.rtc.enabled = true;
#else
    settings.rtc.enabled = false;
#endif
}

void System::setDefaultAprsReceived() {
    for (auto &[callsign, time, rssi, snr, content, count, digipeaterCallsign, digipeaterCount] : aprsReceived) {
        memset(callsign, '\0', CALLSIGN_LENGTH);
        memset(content, '\0', MAX_PACKET_LENGTH);
        memset(digipeaterCallsign, '\0', MAX_PACKET_LENGTH);
        count = 0;
        digipeaterCount = 0;
        rssi = 0;
        snr = 0;
        time = 0;
    }
}


DateTime System::getDateTime() const {
    datetime_t datetime;
    rtc_get_datetime(&datetime);

    int16_t year = datetime.year + 1900;
    int8_t month = datetime.month;
    int8_t day = datetime.day;
    int8_t hour = datetime.hour;

    const auto isDSTNow = isDST(year, month, day, hour);
    addHours(year, month, day, hour, isDSTNow ? 2 : 1);

    return {static_cast<uint16_t>(year), static_cast<uint8_t>(month), static_cast<uint8_t>(day), static_cast<uint8_t>(hour), static_cast<uint8_t>(datetime.min), static_cast<uint8_t>(datetime.sec)};
}

void System::printSettingsAndAprsReceived() {
    for (const auto &config : settingsGetSetFunctions) {
        switch (config.type) {
            case Boolean:
                Log.traceln(F("[CONFIG] %s = %T"), config.name, *static_cast<bool *>(config.pointer));
                break;
            case Int8:
                Log.traceln(F("[CONFIG] %s = %d"), config.name, *static_cast<int8_t *>(config.pointer));
                break;
            case Int16:
                Log.traceln(F("[CONFIG] %s = %u"), config.name, *static_cast<int16_t *>(config.pointer));
                break;
            case Int32:
                Log.traceln(F("[CONFIG] %s = %u"), config.name, *static_cast<int32_t *>(config.pointer));
                break;
            case Int64:
                Log.traceln(F("[CONFIG] %s = %u"), config.name, *static_cast<int64_t *>(config.pointer));
                break;
            case UInt8:
                Log.traceln(F("[CONFIG] %s = %d"), config.name, *static_cast<uint8_t *>(config.pointer));
                break;
            case UInt16:
                Log.traceln(F("[CONFIG] %s = %u"), config.name, *static_cast<uint16_t *>(config.pointer));
                break;
            case UInt32:
                Log.traceln(F("[CONFIG] %s = %u"), config.name, *static_cast<uint32_t *>(config.pointer));
                break;
            case UInt64:
                Log.traceln(F("[CONFIG] %s = %u"), config.name, *static_cast<uint64_t *>(config.pointer));
                break;
            case Char:
                Log.traceln(F("[CONFIG] %s = %c"), config.name, *static_cast<char *>(config.pointer));
                break;
            case Float:
                Log.traceln(F("[CONFIG] %s = %F"), config.name, *static_cast<float *>(config.pointer));
                break;
            case Double:
                Log.traceln(F("[CONFIG] %s = %D"), config.name, *static_cast<double *>(config.pointer));
                break;
            case CharString:
                Log.traceln(F("[CONFIG] %s = %s"), config.name, *static_cast<char* *>(config.pointer));
                break;
        }
    }

    uint8_t frameIndex = 0;
    for (auto &[callsign, time, rssi, snr, content, count, digipeaterCallsign, digipeaterCount] : aprsReceived) {
        if (strlen(callsign) > 0 && strlen(content)) {
            getDateTimeStringFromEpoch(time, bufferText, BUFFER_LENGTH);
            Log.traceln(F("[CONFIG] APRS Frame received #%d at %s from %s with SNR %F and RSSI %F, content: %s. Digi (%d) and last via %s. Count total %u"), frameIndex++, bufferText, callsign, snr, rssi, content, digipeaterCount, digipeaterCallsign, count);
        }
    }

    Log.traceln(F("[SYSTEM] Internal watchdog caused reboot: %T"), watchdog_enable_caused_reboot());

    const auto epoch = getDateTime().unixtime();
    getDateTimeStringFromEpoch(epoch, bufferText, BUFFER_LENGTH);
    Log.infoln(F("[RTC] Date now %s"), bufferText);
}

void System::planReboot() {
    Log.warningln(F("[SYSTEM] Plan reboot"));
    timerReboot.restart();
}

void System::planDfu() {
    Log.warningln(F("[SYSTEM] Plan DFU"));
    timerDfu.restart();
}

void System::printJson(const bool onUsb) {
    JsonWriter *jsonWriter = onUsb ? &serialJsonWriter : &serialLinuxJsonWriter;

    auto json = &jsonWriter->beginObject()
            .property(F("uptime"), millis() / 1000)
            .property(F("time"), getDateTime().unixtime())
            .beginObject(F("errors"))
                .property(F("hasErrors"), hasError())
                .property(F("rtc"), isRtcHasError())
                .property(F("lora"), radio.hasError())
                .property(F("energy"), energyThread->hasError())
                .property(F("weather"), weatherThread->hasError())
            .endObject()
            .beginObject(F("energy"))
                .property(F("nextRun"), static_cast<uint32_t>(energyThread->timeBeforeRun()) / 1000)
                .property(F("percentageBattery"), energyThread->getBatteryPercentage())
                .property(F("voltageBattery"), energyThread->getVoltageBattery())
                .property(F("currentBattery"), energyThread->getCurrentBattery())
                .property(F("voltageSolar"), energyThread->getVoltageSolar())
                .property(F("currentSolar"), energyThread->getCurrentSolar())
            .endObject()
            .beginObject(F("box"));

    json = &json->property(F("temperature"), getTemperatureBox());

    if (settings.energy.type == mpptchg && !energyThread->hasError()) {
        const auto energyThreadMppt = static_cast<EnergyMpptChgThread*>(energyThread);
        json = &json->property(F("alertShutdown"), energyThreadMppt->isAlert());
    }

    json = &json->endObject()
            .beginObject(F("weather"))
                .property(F("nextRun"), static_cast<uint32_t>(weatherThread->timeBeforeRun()) / 1000)
                .property(F("temperature"), weatherThread->enabled ? weatherThread->getTemperature() : 0)
                .property(F("humidity"), weatherThread->enabled ? weatherThread->getHumidity() : 0)
                .property(F("pressure"), weatherThread->enabled ? weatherThread->getPressure() : 0)
            .endObject()
            .beginObject(F("aprsSender"))
                .property(F("sendPositionNextRun"), static_cast<uint32_t>(sendPositionThread->timeBeforeRun()) / 1000)
                .property(F("sendTelemetriesNextRun"), static_cast<uint32_t>(sendTelemetriesThread->timeBeforeRun()) / 1000)
                .property(F("sendStatusNextRun"), static_cast<uint32_t>(sendStatusThread->timeBeforeRun()) / 1000);

    if (sendMeshtasticAprsThread->enabled) {
        json = &json->property(F("sendMeshtasticNextRun"), static_cast<uint32_t>(sendMeshtasticAprsThread->timeBeforeRun()) / 1000);
    }
    if (sendLinuxAprsThread->enabled) {
        json = &json->property(F("sendMeshtasticNextRun"), static_cast<uint32_t>(sendLinuxAprsThread->timeBeforeRun()) / 1000);
    }

    json = &json->endObject()
            .beginObject(F("watchdog"));

    if (settings.energy.type == mpptchg && watchdogSlaveMpptChgThread->enabled) {
        json = &json->beginObject(F("mppt"))
                .property(F("nextRun"), static_cast<uint32_t>(watchdogSlaveMpptChgThread->timeBeforeRun()) / 1000)
                .property(F("lastFed"), static_cast<uint32_t>(watchdogSlaveMpptChgThread->timeSinceFed()) / 1000)
            .endObject();
    }

    if (watchdogSlaveLoraTxThread->enabled) {
        json = &json->beginObject(F("loraTx"))
                .property(F("nextRun"), static_cast<uint32_t>(watchdogSlaveLoraTxThread->timeBeforeRun()) / 1000)
                .property(F("lastFed"), static_cast<uint32_t>(watchdogSlaveLoraTxThread->timeSinceFed()) / 1000)
                .property(F("rxQueue"), radio.countRxItemQueued())
                .property(F("txQueue"), radio.countTxItemQueued())
                .property(F("nbRx"), radio.countRx())
                .property(F("nbTx"), radio.countTx())
            .endObject();
    }

    if (watchdogMeshtastic->enabled) {
        json = &json->beginObject(F("meshtastic"))
                .property(F("nextRun"), static_cast<uint32_t>(watchdogMeshtastic->timeBeforeRun()) / 1000)
                .property(F("lastFed"), static_cast<uint32_t>(watchdogMeshtastic->timeSinceFed()) / 1000)
            .endObject();
    }

    if (watchdogLinux->enabled) {
        json = &json->beginObject(F("linux"))
                .property(F("nextRun"), static_cast<uint32_t>(watchdogLinux->timeBeforeRun()) / 1000)
                .property(F("lastFed"), static_cast<uint32_t>(watchdogLinux->timeSinceFed()) / 1000)
            .endObject();
    }

    json = &json->endObject().beginArray(F("aprsReceived"));

    for (auto &[callsign, time, rssi, snr, content, count, digipeaterCallsign, digipeaterCount] : aprsReceived) {
        if (strlen(callsign) > 0 && strlen(content)) {
            json = &json->beginObject()
            .property(F("callsign"), callsign)
                .property(F("time"), static_cast<uint32_t>(time))
                // .property(F("packet"), content)
                .property(F("snr"), snr)
                .property(F("rssi"), rssi)
                .property(F("count"), static_cast<uint32_t>(count))
                .property(F("digipeaterCount"), digipeaterCount)
                .property(F("digipeaterCallsign"), digipeaterCallsign)
            .endObject();
        }
    }

    json->endArray();

    json = &json->beginObject("config");

    for (const auto &config : settingsGetSetFunctions) {
        switch (config.type) {
            case Boolean:
                json = &json->property(config.name, *static_cast<bool *>(config.pointer));
            break;
            case Int8:
                json = &json->property(config.name, *static_cast<int8_t *>(config.pointer));
            break;
            case Int16:
                json = &json->property(config.name, *static_cast<int16_t *>(config.pointer));
            break;
            case Int32:
                json = &json->property(config.name, *static_cast<int32_t *>(config.pointer));
            break;
            case Int64:
                json = &json->property(config.name, *static_cast<int32_t *>(config.pointer));
            break;
            case UInt8:
                json = &json->property(config.name, *static_cast<uint8_t *>(config.pointer));
            break;
            case UInt16:
                json = &json->property(config.name, *static_cast<uint16_t *>(config.pointer));
            break;
            case UInt32:
                json = &json->property(config.name, *static_cast<uint32_t *>(config.pointer));
            break;
            case UInt64:
                json = &json->property(config.name, *static_cast<uint32_t *>(config.pointer));
            break;
            case Char:
                json = &json->property(config.name, *static_cast<char *>(config.pointer));
            break;
            case Float:
                json = &json->property(config.name, *static_cast<float *>(config.pointer));
            break;
            case Double:
                json = &json->property(config.name, *static_cast<double *>(config.pointer));
            break;
            case CharString:
                json = &json->property(config.name, *static_cast<char* *>(config.pointer));
            break;
        }
    }

    json->endObject();

    if (onUsb) {
        Serial.println();
    }
}

void System::sendToKissInterface(const uint8_t* data, size_t size) {
#if USE_KISS
    kissPacket = kiss_new_packet(buffer, BUFFER_LENGTH / 2);

    kissPacket.data_length = size;
    size = kiss_encode_packet(kissPacket, buffer, BUFFER_LENGTH * 2);

    Serial2.write(buffer, size);
    Serial2.flush();
#endif
}

double System::getTemperatureBox() {
    double temperatureBox = 0;
    double temperatureBoxNb = 0;

    const EnergyMpptChgThread* energyThreadMppt = settings.energy.type == mpptchg && !energyThread->hasError() ?
        static_cast<EnergyMpptChgThread*>(energyThread) : nullptr;

    if (energyThreadMppt != nullptr) {
        temperatureBox += energyThreadMppt->getTemperature();
        temperatureBoxNb++;
    }

    if (settings.rtc.enabled) {
        temperatureBox += rtc.getTemperature();
        temperatureBoxNb++;
    }

    if (temperatureBoxNb == 0) {
        return 0;
    }

    return temperatureBox / temperatureBoxNb;
}

void System::setClock(const bool slow) {
    if (!DISABLE_SLOW_CLOCK && slow) {
        /* Set the system frequency to 18 MHz. */
        set_sys_clock_khz(18 * KHZ, false);
        /* The previous line automatically detached clk_peri from clk_sys, and
           attached it to pll_usb. We need to attach clk_peri back to system PLL to keep SPI
           working at this low speed.
           For details see https://github.com/jgromes/RadioLib/discussions/938
        */
        clock_configure(clk_peri,
                        0,                                                // No glitchless mux
                        CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, // System PLL on AUX mux
                        18 * MHZ,                                         // Input frequency
                        18 * MHZ                                          // Output (must be same as no divider)
        );
        /* Run also ADC on lower clk_sys. */
        clock_configure(clk_adc, 0, CLOCKS_CLK_ADC_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, 18 * MHZ, 18 * MHZ);
        /* Run RTC from XOSC since USB clock is off */
        clock_configure(clk_rtc, 0, CLOCKS_CLK_RTC_CTRL_AUXSRC_VALUE_XOSC_CLKSRC, 12 * MHZ, 47 * KHZ);
        vreg_set_voltage(VREG_VOLTAGE_0_90);
        /* Turn off USB PLL */
        pll_deinit(pll_usb);
    } else {
        Serial.begin(115200);
        Log.begin(LOG_LEVEL_TRACE, &Serial, true, true);
        delay(2500); // Wait for serial debug
        Log.infoln(F("[MAIN] Debug mode"));
    }

    Serial1.begin(115200);
#if USE_KISS
    Serial2.begin(115200);
#endif
}

GpioPin *System::getGpio(const uint8_t pin) {
    if (pin == 0) {
        return nullptr;
    }

    for (const auto gpio : gpiosPin) {
        if (gpio == nullptr) {
            continue;
        }

        if (gpio->getPin() == pin) {
            return gpio;
        }
    }

    return nullptr;
}

GpioPin * System::getGpio(const char *name) {
    if (strlen(name) == 0) {
        return nullptr;
    }

    for (const auto gpio : gpiosPin) {
        if (gpio == nullptr) {
            continue;
        }

        if (strcmp(gpio->getName(), name) == 0) {
            return gpio;
        }
    }

    return nullptr;
}