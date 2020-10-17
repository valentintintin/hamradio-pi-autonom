import { Observable, of } from 'rxjs';
import { LogService } from './log.service';
import { catchError, map, tap } from 'rxjs/operators';
import { CommunicationMpptchdService } from './communication-mpptchd.service';
import { DatabaseService } from './database.service';
import { Sensors } from '../models/sensors';
import { GpioService } from './gpio.service';
import { SerialService } from './serial.service';
import fs = require('fs');

const si = require('systeminformation');

export class SensorsService {

    public static getMpptChgData(): Observable<Sensors> {
        LogService.log('sensors', 'Start getting all MpptChg');

        return CommunicationMpptchdService.instance.getStatus().pipe(
            map(data => {
                const sensorsObject = new Sensors();
                sensorsObject.voltageBattery = data.values.batteryVoltage;
                sensorsObject.voltageSolar = data.values.solarVoltage;
                sensorsObject.currentBattery = data.values.batteryCurrent;
                sensorsObject.currentSolar = data.values.solarCurrent;
                sensorsObject.currentCharge = data.values.chargeCurrent;
                sensorsObject.temperatureBattery = data.values.internalThermometer / 10;
                sensorsObject.voltageBattery = data.values.batteryVoltage;
                sensorsObject.rawMpptchg = JSON.stringify(data);
                return sensorsObject;
            }),
            catchError(err => {
                LogService.log('sensors', 'Get all mpptChd KO', err);
                return of(new Sensors());
            })
        )
    }

    public static getTemperatureRtc(): number {
        try {
            const result = GpioService.USE_FAKE ? 20.5 : parseInt(fs.readFileSync('/sys/bus/i2c/devices/i2c-0/0-0068/hwmon/hwmon1/temp1_input', 'utf8'), 10) / 1000;
            LogService.log('sensors', 'Get temperature RTC', result);
            return result;
        } catch (e) {
            LogService.log('sensors', 'Get temperature RTC KO', e);
            return -1;
        }
    }

    public static getUptime(): number {
        LogService.log('sensors', 'Get uptime', si.time().uptime);
        return si.time().uptime;
    }

    public static getAllCurrent(): Observable<Sensors> {
        LogService.log('sensors', 'Get all');

        return SensorsService.getMpptChgData().pipe(
            map(datas => {
                datas.temperatureRtc = SensorsService.getTemperatureRtc();
                return datas;
            }),
            map(datas => {
                datas.uptime = SensorsService.getUptime();
                return datas;
            }),
            map(datas => {
                const stringSerial = SerialService.instance.data;
                if (stringSerial) {
                    LogService.log('sensors', 'Get from Serial', stringSerial);

                    const datasSplitted = stringSerial.split('[');
                    datasSplitted.forEach(split => {
                        const indexCrochet = split.indexOf(']');
                        const dataSplit = +split.substring(indexCrochet + 1);
                        const dataName = split.substring(0, split.indexOf(']'));
                        switch (dataName) {
                            case 'LIGHT':
                                datas.light = dataSplit;
                                break;
                            case 'PRESSURE_TEMP':
                                datas.temperaturePressure = dataSplit;
                                break;
                            case 'PRESSURE':
                                datas.pressure = dataSplit;
                                break;
                            case 'TEMP':
                                datas.temperature = dataSplit;
                                break;
                            case 'HUMIDITY':
                                datas.humidity = dataSplit;
                                break;
                            default:
                                LogService.log('sensors', 'Data name unkown', {
                                    string: split,
                                    dataName
                                });
                        }
                    });
                }
                LogService.log('sensors', 'Get all OK', datas);
                return datas;
            }),
        );
    }

    public static getAllCurrentAndSave(): Observable<Sensors> {
        return this.getAllCurrent().pipe(
            tap(data => this.save(data))
        )
    }

    public static getLast(): Observable<Sensors> {
        return DatabaseService.selectLast<Sensors>(Sensors.name).pipe(tap(data => {
            if (data) {
                (data as any).createdAt = new Date(data.createdAt);
                delete (data as any).rawMpptchg;
                delete (data as any).id;
            }
        }));
    }

    public static getAllSaved(limit: number = 0, skip: number = 0): Observable<Sensors[]> {
        return DatabaseService.selectAll<Sensors>(Sensors.name, limit, skip).pipe(
            map(datas => datas.filter(data => data.createdAt >= 1577833200000)),
            tap(datas => {
                if (datas.length > 0) {
                    datas.forEach(data => {
                        (data as any).createdAt = new Date(data.createdAt);
                        delete (data as any).rawMpptchg;
                    });
                }
            })
        );
    }

    private static save(datas: Sensors): void {
        DatabaseService.insert(datas).subscribe();
    }
}
