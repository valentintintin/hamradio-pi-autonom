import { Observable, Observer, of } from 'rxjs';
import { LogService } from './log.service';
import { map, switchMap, tap } from 'rxjs/operators';
import { CommunicationMpptchdService } from './communication-mpptchd.service';
import { SensorsConfigInterface } from '../config/sensors-config.interface';
import { debug } from '../index';
import { DatabaseService } from './database.service';
import { Sensors } from '../models/sensors';
import fs = require('fs');

const si = require('systeminformation');

export class SensorsService {

    public static getMpptChgData(): Observable<Sensors> {
        LogService.log('sensors', 'Get all');

        return CommunicationMpptchdService.instance.getStatus().pipe(map(data => {
                const sensorsObject = new Sensors();
                sensorsObject.voltageBattery = data.values.batteryVoltage;
                sensorsObject.voltageSolar = data.values.solarVoltage;
                sensorsObject.currentBattery = data.values.batteryCurrent;
                sensorsObject.currentSolar = data.values.solarCurrent;
                sensorsObject.currentCharge = data.values.chargeCurrent;
                sensorsObject.alertAsserted = data.alertAsserted ? 1 : 0;
                sensorsObject.nightDetected = data.nightDetected ? 1 : 0;
                sensorsObject.temperatureBattery = data.values.internalThermometer;
                sensorsObject.rawMpptchg = JSON.stringify(data);
                sensorsObject.voltageBattery = data.values.batteryVoltage;
                return sensorsObject;
            })
        )
    }

    public static getTemperatureCpu(): Observable<number> {
        return new Observable<number>((observer: Observer<number>) => {
            LogService.log('sensors', 'Get temperature CPU');

            si.cpuTemperature().then(data => {
                observer.next(data.main);
                observer.complete();
            }).catch(err => {
                LogService.log('sensors', 'Get temperature CPU KO', err);
                return of(-1);
            });
        });
    }

    public static getTemperatureRtc(): number {
        LogService.log('sensors', 'Get temperature RTC');

        if (debug) {
            return 20.5;
        }

        try {
            return parseInt(fs.readFileSync('/sys/bus/i2c/devices/1-0068/hwmon/hwmon1/temp1_input', 'utf8'), 10) / 1000;
        } catch (e) {
            LogService.log('sensors', 'Get temperature RTC KO', e);
            return -1;
        }
    }

    public static getUptime(): number {
        LogService.log('sensors', 'Get uptime');
        return si.time().uptime;
    }

    public static getAll(): Observable<Sensors> {
        LogService.log('sensors', 'Start get all');

        return SensorsService.getMpptChgData().pipe(
            switchMap(datas => SensorsService.getTemperatureCpu().pipe(map(data => {
                datas.temperatureCpu = data;
                return datas;
            }))),
            map(datas => {
                datas.temperatureRtc = SensorsService.getTemperatureRtc();
                return datas;
            }),
            map(datas => {
                datas.uptime = SensorsService.getUptime();
                return datas;
            }),
        );
    }

    public static getAllAndSave(config: SensorsConfigInterface): Observable<Sensors> {
        return this.getAll().pipe(
            tap(data => this.save(data, config.csvPath))
        )
    }

    private static save(datas: Sensors, path: string): void {
        DatabaseService.insert(datas).subscribe(_ => {
            delete datas.rawMpptchg;

            if (!fs.existsSync(path)) {
                fs.writeFileSync(path, 'date,' + Object.keys(datas).join(',') + '\n');
                LogService.log('sensors', 'CSV created', path);
            }

            fs.appendFileSync(path, new Date().toLocaleString() + ',' + Object.values(datas).join(',') + '\n');
            LogService.log('sensors', 'CSV saved with datas', path);
        });
    }
}
