#include <hardware/rtc.h>
#include "I2CSlave.h"
#include "System.h"

#include "ArduinoLog.h"

uint8_t I2CSlave::currentRegToRead = 0;
System *I2CSlave::system = nullptr;
char I2CSlave::commandToSendToMaster[BUFFER_LENGTH + 1];
char I2CSlave::commandResponseFromMaster[BUFFER_LENGTH + 1];
char I2CSlave::commandReceivedFromMaster[BUFFER_LENGTH + 1];
char I2CSlave::commandResponseToSendToMaster[BUFFER_LENGTH + 1];

void I2CSlave::begin(System *system) {
    I2CSlave::system = system;

    if (!Wire1.setSDA(2)) {
        Log.errorln(F("[I2C_SLAVE] Begin KO. Wrong SDA pin"));
        return;
    }

    if (!Wire1.setSCL(3)) {
        Log.errorln(F("[I2C_SLAVE] Begin KO. Wrong SCL pin"));
        return;
    }

    const auto address = system->settings.i2c.address;

    Wire1.begin(address);
    Wire1.onRequest(onRequest);
    Wire1.onReceive(onReceive);

    Log.infoln(F("[I2C_SLAVE] Begin on address %x and pin SDA 2, SCL 3"), address);
}

void I2CSlave::end() {
    Log.infoln(F("[I2C_SLAVE] Stopped"));
    Wire1.end();
}

void I2CSlave::sendCommandToMaster(const char *command) {
    strncpy(commandToSendToMaster, command, BUFFER_LENGTH);
}

void I2CSlave::onRequest() {
    const Settings settings = system->settings;
    uint32_t value = 0;

    const DateTime datetime = system->getDateTime();

    switch (currentRegToRead) {
        case REG_PING:
            value += HAS_POWER;

            if (system->weatherThread->enabled) {
                value += HAS_ENVIRONMENT;
            }

            if (settings.rtc.enabled) {
                value += HAS_DATETIME;
            }
            break;
        case REG_BATTERY_VOLTAGE:
            value = !system->energyThread->hasError() ? system->energyThread->getVoltageBattery() : 0;
            break;
        case REG_BATTERY_CURRENT:
            value = !system->energyThread->hasError() ? system->energyThread->getCurrentBattery() : 0;
            break;
        case REG_SOLAR_VOLTAGE:
            value = !system->energyThread->hasError() ? system->energyThread->getVoltageSolar() : 0;
            break;
        case REG_SOLAR_CURRENT:
            value = !system->energyThread->hasError() ? system->energyThread->getCurrentSolar() : 0;
            break;
        case REG_TEMPERATURE:
            value = system->weatherThread->enabled && !system->weatherThread->hasError() ? static_cast<int16_t>(system->weatherThread->getTemperature() * 100.0) : 0;
            break;
        case REG_PRESSURE:
            value = system->weatherThread->enabled && !system->weatherThread->hasError() ? static_cast<int16_t>(system->weatherThread->getPressure()) : 0;
            break;
        case REG_HUMIDITY:
            value = system->weatherThread->enabled && !system->weatherThread->hasError() ? static_cast<int16_t>(system->weatherThread->getHumidity()) : 0;
            break;
        case REG_SECONDS:
            value = datetime.second();
            break;
        case REG_MINUTES:
            value = datetime.minute();
            break;
        case REG_HOURS:
            value = datetime.hour();
            break;
        case REG_DAYS:
            value = datetime.day();
            break;
        case REG_MONTHS:
            value = datetime.month();
            break;
        case REG_YEARS:
            value = datetime.year();
            break;
        case REG_COMMAND_RECEIVE_FROM_SLAVE:
            Log.infoln(F("[I2C_SLAVE] Send %s for register %x (command to master)"), commandToSendToMaster, currentRegToRead);
            Wire1.write(commandToSendToMaster);
            commandToSendToMaster[0] = '\0';
            return;
        case REG_COMMAND_RESPONSE_TO_MASTER:
            Log.infoln(F("[I2C_SLAVE] Send %s for register %x (command response to master)"), commandToSendToMaster, currentRegToRead);
            Wire1.write(commandResponseToSendToMaster);
            commandResponseToSendToMaster[0] = '\0';
        return;
        default:
            Log.warningln(F("[I2C_SLAVE] Register %x not found"), currentRegToRead);
            break;
    }

    Log.infoln(F("[I2C_SLAVE] Send %d for register %x"), value, currentRegToRead);

    Wire1.write(value >> 8 & 0xFF);
    Wire1.write(value & 0xFF);
}


void I2CSlave::onReceive(const int bytes) {
    Log.traceln(F("[I2C_SLAVE] Receive %d bytes"), bytes);

    if (bytes == 1) {
        currentRegToRead = Wire1.read();

        if (currentRegToRead == REG_COMMAND_RESPONSE_TRANSMIT_FROM_MASTER) {
            Log.traceln(F("[I2C_SLAVE] Receive value %x to write (command response from master)"), currentRegToRead);
            byte i = 0;
            while (Wire.available() && i < BUFFER_LENGTH) {
                commandResponseFromMaster[i++] = Wire.read();
            }
            commandResponseFromMaster[i] = '\0';
        } else if (currentRegToRead == REG_COMMAND_RECEIVED_FROM_MASTER) {
            Log.traceln(F("[I2C_SLAVE] Receive value %x to write (command received from master)"), currentRegToRead);
            byte i = 0;
            while (Wire.available() && i < BUFFER_LENGTH) {
                commandReceivedFromMaster[i++] = Wire.read();
            }
            commandReceivedFromMaster[i] = '\0';

            system->command.processCommand(nullptr, commandReceivedFromMaster);
            commandReceivedFromMaster[0] = '\0';

            strncpy(commandResponseToSendToMaster, system->command.response, BUFFER_LENGTH);
        } else {
            Log.traceln(F("[I2C_SLAVE] Receive value %x to read"), currentRegToRead);
        }

        if (system->watchdogMeshtastic->enabled) {
            system->watchdogMeshtastic->feed();
        }
    } else {
        Log.warningln(F("[I2C_SLAVE] Receive size 0"));
    }
}