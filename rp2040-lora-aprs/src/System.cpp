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
#include "Threads/LdrBoxOpenedThread.h"

#include "I2CSlave.h"
#include "utils.h"
#include "PicoSleep.h"

System::System() : communication(this), command(this) {
    timerReboot.pause();
    timerDfu.pause();
}

bool System::begin() {
    Log.infoln(F("[SYSTEM] Starting"));

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

//    setDefaultSettings();
//    saveSettings();

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
        gpiosPin[gpioI++] = new GpioPin(settings.rtc.wakeUpPin, INPUT);
        const auto now = RTClib::now();
        if (now.year() >= 2025) {
            setTimeToInternalRtc(now.unixtime());
        } else {
            Log.warningln(F("[RTC] Wrong rtc time !"));
        }
    }

    communication.begin();

    ldrBoxOpenedThread = new LdrBoxOpenedThread(this);
    threadController.add(ldrBoxOpenedThread);

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

    if (settings.meshtastic.i2cSlaveEnabled) {
        I2CSlave::begin(this);
    }

    watchdogSlaveMpptChgThread = new WatchdogSlaveMpptChgThread(this);
    if (settings.energy.type == mpptchg) {
        threadController.add(watchdogSlaveMpptChgThread);
    }

    watchdogSlaveLoraTxThread = new WatchdogSlaveLoraTxThread(this);
    threadController.add(watchdogSlaveLoraTxThread);

    auto *gpioMeshtastic = new GpioPin(settings.meshtastic.pin, OUTPUT_2MA);
    gpiosPin[gpioI++] = gpioMeshtastic;
    watchdogMeshtastic = new WatchdogMasterPinThread(this, PSTR("MESHTASTIC"), gpioMeshtastic, settings.meshtastic.intervalTimeoutWatchdog, settings.meshtastic.watchdogEnabled);
    threadController.add(watchdogMeshtastic);

    auto *gpioLinuxBoard = new GpioPin(settings.linux.pin, OUTPUT_12MA);
    gpiosPin[gpioI++] = gpioLinuxBoard;
    watchdogLinux = new WatchdogMasterPinThread(this, PSTR("LINUX"), gpioLinuxBoard, settings.linux.intervalTimeoutWatchdog, settings.linux.watchdogEnabled);
    threadController.add(watchdogLinux);

    gpiosPin[gpioI++] = new GpioPin(settings.linux.nprPin, OUTPUT_12MA, false, true);
    gpiosPin[gpioI++] = new GpioPin(settings.linux.wifiPin, OUTPUT_12MA, false, true);

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
    } else if (Serial2.available()) {
        streamReceived = &Serial2;
        Log.traceln(F("Serial UART 1 incoming (KISS)"));

        if (watchdogLinux->enabled) {
            watchdogLinux->feed();
        }
    }

    if (streamReceived != nullptr) {
        gpioLed.setState(true);

        if (streamReceived == &Serial2) {
            kissPacket = kiss_new_packet(buffer, BUFFER_LENGTH);
            size_t bytesRead = 0;
            while (streamReceived->available()) {
                buffer[bytesRead++] = streamReceived->read();

                size_t bytesConsumed = kiss_decode_packet(&kissPacket, buffer, bytesRead);

                bytesRead -= bytesConsumed;

                if (kissPacket.complete_packet) {
                    Log.infoln(F("[SERIAL_KISS] Received %d of data from KISS OK"), kissPacket.data_length);
                    communication.sendRaw(kissPacket.data, kissPacket.data_length);
                }
            }
        } else {
            const size_t lineLength = streamReceived->readBytesUntil('\n', bufferText, BUFFER_LENGTH - 5);
            bufferText[lineLength] = '\0';

            Log.traceln(F("[SERIAL] Received %s"), bufferText);

            command.processCommand(streamReceived, bufferText);
        }

        streamReceived->flush();
    } else {
        threadController.run();
    }

    communication.update();

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
        Log.warningln(F("[CONFIG] Fail to open, we create it"));

        setDefaultSettings();

        return saveSettings();
    }

    file.read(reinterpret_cast<uint8_t *>(&settings), sizeof(Settings));
    file.close();

    Log.infoln(F("[CONFIG] Read correctly"));

    printSettings();

    return true;
}

bool System::saveSettings() {
    File file = LittleFS.open("/config.dat", "w");
    if (!file) {
        Log.errorln(F("[CONFIG] Fail to save, we use the default one"));
        printSettings();
        return false;
    }

    file.write(reinterpret_cast<uint8_t *>(&settings), sizeof(Settings));
    file.close();

    Log.infoln(F("[CONFIG] Save to FS"));

    return true;
}

void System::addAprsFrameReceivedToHistory(const AprsPacketLite *packet, const float snr, const float rssi) {
    uint8_t frameIndex = 0;

    for (const auto oldFrame : settings.aprsCallsignsHeard) {
        if (strcmp(oldFrame.callsign, packet->source) == 0 || strlen(oldFrame.callsign) == 0) {
            break;
        }

        frameIndex++;
    }

    lastAprsHeard = &settings.aprsCallsignsHeard[frameIndex];

    lastAprsHeard->time = getDateTime().unixtime();
    lastAprsHeard->snr = snr;
    lastAprsHeard->rssi = rssi;
    lastAprsHeard->count++;
    lastAprsHeard->digipeaterCount = packet->digipeaterCount;
    strcpy(lastAprsHeard->callsign, packet->source);
    strcpy(lastAprsHeard->content, packet->raw);
    strcpy(lastAprsHeard->digipeaterCallsign, packet->lastDigipeaterCallsignInPath);

    saveSettings();
}

bool System::resetSettings() {
    if (!LittleFS.format()) {
        Log.errorln(F("[CONFIG] Fail to format"));
        return false;
    }

    Log.infoln(F("[CONFIG] Deleted"));

    loadSettings();

    return true;
}

void System::setDefaultSettings() {
    settings.useInternalWatchdog = true;
    settings.useSlowClock = true;

    settings.lora.frequency = 433.775;
    settings.lora.bandwidth = 125;
    settings.lora.spreadingFactor = 12;
    settings.lora.codingRate = 5;
    settings.lora.outputPower = 12;
    settings.lora.txEnabled = true;
    settings.lora.watchdogTxEnabled = true;
    settings.lora.intervalTimeoutWatchdogTx = 7200000; // 2 hours

    strcpy_P(settings.aprs.call, PSTR("F4HVV-15"));
    strcpy_P(settings.aprs.destination, PSTR("APLV1"));
    strcpy_P(settings.aprs.path, PSTR("WIDE1-1"));
    settings.aprs.symbol = '#';
    settings.aprs.symbolTable = 'L';
    settings.aprs.latitude = 45.325776;
    settings.aprs.longitude = 5.636580;
    settings.aprs.altitude = 850;
    settings.aprs.comment[0] = '\0';
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

    settings.meshtastic.watchdogEnabled = true;
    settings.meshtastic.intervalTimeoutWatchdog = 300000; // 5 minutes
    settings.meshtastic.pin = 10;
    settings.meshtastic.i2cSlaveEnabled = true;
    settings.meshtastic.i2cSlaveAddress = 0x11;
    settings.meshtastic.aprsSendItemEnabled = true;
    settings.meshtastic.intervalSendItem = 3600000; // 1 hour
    strcpy_P(settings.meshtastic.itemName, PSTR("MSH"));
    settings.meshtastic.symbol = '#';
    settings.meshtastic.symbolTable = '\\';
    strcpy_P(settings.meshtastic.itemComment, PSTR("Meshtastic LongModerate 869.4625"));
    settings.meshtastic.latitude = 45.325734;
    settings.meshtastic.longitude = 5.636680;
    settings.meshtastic.altitude = 850;

    settings.mpptWatchdog.enabled = true;
    settings.mpptWatchdog.timeout = 90; // 1,5 minutes
    settings.mpptWatchdog.timeOff = 10; // 10 seconds
    settings.mpptWatchdog.intervalFeed = 30000; // 30 seconds

    settings.energy.type = mpptchg;
    settings.energy.intervalCheck = 30000; // 30 seconds
    settings.energy.mpptPowerOffVoltage = 11550;
    settings.energy.mpptPowerOnVoltage = 12000;
    settings.energy.sendAprsMessageWhenAlert = true;

    settings.weather.enabled = true;
    settings.weather.intervalCheck = 60000; // 60 seconds

    settings.linux.watchdogEnabled = true;
    settings.linux.intervalTimeoutWatchdog = 1200000; // 20 minutes
    settings.linux.pin = 9;
    settings.linux.wifiPin = 11;
    settings.linux.nprPin = 12;
    settings.linux.aprsSendItemEnabled = false;
    settings.linux.intervalSendItem = 3600000; // 1 hour
    strcpy_P(settings.linux.itemName, PSTR("CAMIP"));
    settings.linux.symbol = 'I';
    settings.linux.symbolTable = '/';
    strcpy_P(settings.linux.itemComment, PSTR("f4hvv.valentin-saugnier.fr/f4hvv-15"));
    settings.linux.latitude = 45.325786;
    settings.linux.longitude = 5.636669;
    settings.linux.altitude = 850;

    settings.rtc.enabled = true;
    settings.rtc.wakeUpPin = 6;

    settings.boxOpened.enabled = false;
    settings.boxOpened.pin = 7;
    settings.boxOpened.intervalCheck = 120000; // 2 minutes
}

DateTime System::getDateTime() const {
    datetime_t datetime;
    if (settings.rtc.enabled) {
        rtc_get_datetime(&datetime);
    }

    return DateTime(datetime.year + 1900, datetime.month, datetime.day, datetime.hour, datetime.min, datetime.sec);
}

void System::printSettings() {
    Log.traceln(F("[CONFIG] lora.frequency = %F"), settings.lora.frequency);
    Log.traceln(F("[CONFIG] lora.bandwidth = %u"), settings.lora.bandwidth);
    Log.traceln(F("[CONFIG] lora.spreadingFactor = %u"), settings.lora.spreadingFactor);
    Log.traceln(F("[CONFIG] lora.codingRate = %u"), settings.lora.codingRate);
    Log.traceln(F("[CONFIG] lora.outputPower = %u"), settings.lora.outputPower);
    Log.traceln(F("[CONFIG] lora.txEnabled = %T"), settings.lora.txEnabled);
    Log.traceln(F("[CONFIG] lora.watchdogTxEnabled = %T"), settings.lora.watchdogTxEnabled);
    Log.traceln(F("[CONFIG] lora.intervalTimeoutWatchdogTx = %u"), settings.lora.intervalTimeoutWatchdogTx);

    Log.traceln(F("[CONFIG] aprs.call = %s"), settings.aprs.call);
    Log.traceln(F("[CONFIG] aprs.destination = %s"), settings.aprs.destination);
    Log.traceln(F("[CONFIG] aprs.path = %s"), settings.aprs.path);
    Log.traceln(F("[CONFIG] aprs.comment = %s"), settings.aprs.comment);
    Log.traceln(F("[CONFIG] aprs.status = %s"), settings.aprs.status);
    Log.traceln(F("[CONFIG] aprs.symbol = %c"), settings.aprs.symbol);
    Log.traceln(F("[CONFIG] aprs.symbolTable = %c"), settings.aprs.symbolTable);
    Log.traceln(F("[CONFIG] aprs.latitude = %D"), settings.aprs.latitude);
    Log.traceln(F("[CONFIG] aprs.longitude = %D"), settings.aprs.longitude);
    Log.traceln(F("[CONFIG] aprs.altitude = %u"), settings.aprs.altitude);
    Log.traceln(F("[CONFIG] aprs.digipeaterEnabled = %T"), settings.aprs.digipeaterEnabled);
    Log.traceln(F("[CONFIG] aprs.telemetryEnabled = %T"), settings.aprs.telemetryEnabled);
    Log.traceln(F("[CONFIG] aprs.intervalTelemetry = %u"), settings.aprs.intervalTelemetry);
    Log.traceln(F("[CONFIG] aprs.statusEnabled = %T"), settings.aprs.statusEnabled);
    Log.traceln(F("[CONFIG] aprs.intervalStatus = %u"), settings.aprs.intervalStatus);
    Log.traceln(F("[CONFIG] aprs.positionWeatherEnabled = %T"), settings.aprs.positionWeatherEnabled);
    Log.traceln(F("[CONFIG] aprs.intervalPositionWeather = %u"), settings.aprs.intervalPositionWeather);
    Log.traceln(F("[CONFIG] aprs.telemetryInPosition = %d"), settings.aprs.telemetryInPosition);
    Log.traceln(F("[CONFIG] aprs.telemetrySequenceNumber = %d"), settings.aprs.telemetrySequenceNumber);

    Log.traceln(F("[CONFIG] meshtastic.watchdogEnabled = %T"), settings.meshtastic.watchdogEnabled);
    Log.traceln(F("[CONFIG] meshtastic.intervalTimeoutWatchdog = %u"), settings.meshtastic.intervalTimeoutWatchdog);
    Log.traceln(F("[CONFIG] meshtastic.pin = %u"), settings.meshtastic.pin);
    Log.traceln(F("[CONFIG] meshtastic.i2cSlaveEnabled = %T"), settings.meshtastic.i2cSlaveEnabled);
    Log.traceln(F("[CONFIG] meshtastic.i2cSlaveAddress = %X"), settings.meshtastic.i2cSlaveAddress);
    Log.traceln(F("[CONFIG] meshtastic.aprsSendItemEnabled = %T"), settings.meshtastic.aprsSendItemEnabled);
    Log.traceln(F("[CONFIG] meshtastic.intervalSendItem = %u"), settings.meshtastic.intervalSendItem);
    Log.traceln(F("[CONFIG] meshtastic.itemName = %s"), settings.meshtastic.itemName);
    Log.traceln(F("[CONFIG] meshtastic.itemComment = %s"), settings.meshtastic.itemComment);
    Log.traceln(F("[CONFIG] meshtastic.symbol = %c"), settings.meshtastic.symbol);
    Log.traceln(F("[CONFIG] meshtastic.symbolTable = %c"), settings.meshtastic.symbolTable);

    Log.traceln(F("[CONFIG] mpptWatchdog.enabled = %T"), settings.mpptWatchdog.enabled);
    Log.traceln(F("[CONFIG] mpptWatchdog.timeout = %u"), settings.mpptWatchdog.timeout);
    Log.traceln(F("[CONFIG] mpptWatchdog.intervalFeed = %u"), settings.mpptWatchdog.intervalFeed);
    Log.traceln(F("[CONFIG] mpptWatchdog.timeOff = %u"), settings.mpptWatchdog.timeOff);

    Log.traceln(F("[CONFIG] boxOpened.enabled = %T"), settings.boxOpened.enabled);
    Log.traceln(F("[CONFIG] boxOpened.intervalCheck = %u"), settings.boxOpened.intervalCheck);
    Log.traceln(F("[CONFIG] boxOpened.pin = %u"), settings.boxOpened.pin);

    Log.traceln(F("[CONFIG] weather.enabled = %T"), settings.weather.enabled);
    Log.traceln(F("[CONFIG] weather.intervalCheck = %u"), settings.weather.intervalCheck);

    Log.traceln(F("[CONFIG] energy.intervalCheck = %u"), settings.energy.intervalCheck);
    Log.traceln(F("[CONFIG] energy.type = %d"), settings.energy.type);
    Log.traceln(F("[CONFIG] energy.adcPin = %u"), settings.energy.adcPin);
    Log.traceln(F("[CONFIG] energy.inaChannelBattery = %u"), settings.energy.inaChannelBattery);
    Log.traceln(F("[CONFIG] energy.inaChannelSolar = %u"), settings.energy.inaChannelSolar);
    Log.traceln(F("[CONFIG] energy.mpptPowerOnVoltage = %u"), settings.energy.mpptPowerOnVoltage);
    Log.traceln(F("[CONFIG] energy.mpptPowerOffVoltage = %u"), settings.energy.mpptPowerOffVoltage);
    Log.traceln(F("[CONFIG] energy.sendAprsMessageWhenAlert = %T"), settings.energy.sendAprsMessageWhenAlert);

    Log.traceln(F("[CONFIG] linux.watchdogEnabled = %T"), settings.linux.watchdogEnabled);
    Log.traceln(F("[CONFIG] linux.intervalTimeoutWatchdog = %u"), settings.linux.intervalTimeoutWatchdog);
    Log.traceln(F("[CONFIG] linux.pin = %u"), settings.linux.pin);
    Log.traceln(F("[CONFIG] linux.nprPin = %u"), settings.linux.nprPin);
    Log.traceln(F("[CONFIG] linux.wifiPin = %u"), settings.linux.wifiPin);
    Log.traceln(F("[CONFIG] linux.aprsSendItemEnabled = %T"), settings.linux.aprsSendItemEnabled);
    Log.traceln(F("[CONFIG] linux.intervalSendItem = %lu"), settings.linux.intervalSendItem);
    Log.traceln(F("[CONFIG] linux.itemName = %s"), settings.linux.itemName);
    Log.traceln(F("[CONFIG] linux.itemComment = %s"), settings.linux.itemComment);
    Log.traceln(F("[CONFIG] linux.symbol = %c"), settings.linux.symbol);
    Log.traceln(F("[CONFIG] linux.symbolTable = %c"), settings.linux.symbolTable);

    Log.traceln(F("[CONFIG] rtc.enabled = %T"), settings.rtc.enabled);
    Log.traceln(F("[CONFIG] rtc.wakeUpPin = %u"), settings.rtc.wakeUpPin);

    Log.traceln(F("[CONFIG] useInternalWatchdog = %T"), settings.useInternalWatchdog);

    uint8_t frameIndex = 0;
    for (auto &[callsign, time, rssi, snr, content, count, digipeaterCallsign, digipeaterCount, reserved] : settings.aprsCallsignsHeard) {
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
    const bool isBoxOpened = ldrBoxOpenedThread->enabled && ldrBoxOpenedThread->isBoxOpened(); // Here to avoid log serial
    JsonWriter *jsonWriter = onUsb ? &serialJsonWriter : &serialLinuxJsonWriter;

    auto json = &jsonWriter->beginObject()
            .property(F("uptime"), millis() / 1000)
            .property(F("time"), getDateTime().unixtime())
            .beginObject(F("errors"))
                .property(F("lora"), communication.hasError())
                .property(F("energy"), energyThread->hasError())
                .property(F("weather"), weatherThread->hasError())
            .endObject()
            .beginObject(F("energy"))
                .property(F("nextRun"), static_cast<uint32_t>(energyThread->timeBeforeRun()) / 1000)
                .property(F("voltageBattery"), energyThread->hasError() ? 0 : energyThread->getVoltageBattery())
                .property(F("currentBattery"), energyThread->hasError() ? 0 : energyThread->getCurrentBattery())
                .property(F("voltageSolar"), energyThread->hasError() ? 0 : energyThread->getVoltageSolar())
                .property(F("currentSolar"), energyThread->hasError() ? 0 : energyThread->getCurrentBattery())
            .endObject()
            .beginObject(F("box"));

    if (settings.rtc.enabled) {
        json = &json->property(F("temperatureRtc"), rtc.getTemperature());
    }

    if (settings.energy.type == mpptchg && !energyThread->hasError()) {
        const auto energyThreadMppt = static_cast<EnergyMpptChgThread*>(energyThread);
        json = &json->property(F("temperatureBattery"), energyThreadMppt->getTemperature());
        json = &json->property(F("alertBattery"), energyThreadMppt->isAlert());
    }

    if (ldrBoxOpenedThread->enabled) {
        json = &json->property(F("opened"), isBoxOpened);
    }

    json = &json->endObject()
            .beginObject(F("weather"))
                .property(F("nextRun"), static_cast<uint32_t>(weatherThread->timeBeforeRun()) / 1000)
                .property(F("temperature"), weatherThread->enabled && !weatherThread->hasError() ? weatherThread->getTemperature() : 0)
                .property(F("humidity"), weatherThread->enabled && !weatherThread->hasError() ? weatherThread->getHumidity() : 0)
                .property(F("pressure"), weatherThread->enabled && !weatherThread->hasError() ? weatherThread->getPressure() : 0)
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

    for (auto &[callsign, time, rssi, snr, content, count, digipeaterCallsign, digipeaterCount, reserved] : settings.aprsCallsignsHeard) {
        if (strlen(callsign) > 0 && strlen(content)) {
            json = &json->beginObject()
            .property(F("callsign"), callsign)
                .property(F("time"), static_cast<uint32_t>(time))
                .property(F("packet"), content)
                .property(F("snr"), snr)
                .property(F("rssi"), rssi)
                .property(F("count"), static_cast<uint32_t>(count))
                .property(F("digipeaterCount"), digipeaterCount)
                .property(F("digipeaterCallsign"), digipeaterCallsign)
            .endObject();
        }
    }

    json->endArray().endObject();

    if (onUsb) {
        Serial.println();
    }
}

void System::sendToKissInterface(const uint8_t* data, size_t size) {
    kissPacket = kiss_new_packet(buffer, BUFFER_LENGTH / 2);

    kissPacket.data_length = size;
    size = kiss_encode_packet(kissPacket, buffer, BUFFER_LENGTH * 2);

    Serial2.write(buffer, size);
    Serial2.flush();
}

void System::setClock(const bool slow) {
    isSlowClock = slow;

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
    Serial2.begin(115200);
}

GpioPin *System::getGpio(const uint8_t pin) {
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
