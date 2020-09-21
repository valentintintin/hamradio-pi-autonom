import { Observable, Observer, of } from 'rxjs';
import { LogService } from './log.service';
import { catchError, map, switchMap, tap } from 'rxjs/operators';
import { CommunicationMpptchdService } from './communication-mpptchd.service';
import { SensorsConfigInterface } from '../config/sensors-config.interface';
import { DatabaseService } from './database.service';
import { Sensors } from '../models/sensors';
import { GpioService } from './gpio.service';
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

    public static getTemperatureCpu(): Observable<number> {
        return new Observable<number>((observer: Observer<number>) => {
            LogService.log('sensors', 'Start getting temperature CPU');

            si.cpuTemperature().then(data => {
                LogService.log('sensors', 'Get temperature CPU', data.main);
                observer.next(data.main);
                observer.complete();
            }).catch(err => {
                LogService.log('sensors', 'Get temperature CPU KO', err);
                return of(-1);
            });
        });
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
                LogService.log('sensors', 'Get all OK', datas);
                return datas;
            }),
        );
    }

    public static getAllCurrentAndSave(config: SensorsConfigInterface): Observable<Sensors> {
        return this.getAllCurrent().pipe(
            tap(data => this.save(data, config.csvPath))
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

    public static getAllSaved(limit: number = 0): Observable<Sensors[]> {
        return DatabaseService.selectAll<Sensors>(Sensors.name, limit).pipe(tap(datas => {
            if (datas.length > 0) {
                datas.forEach(data => {
                    (data as any).createdAt = new Date(data.createdAt);
                    delete (data as any).rawMpptchg;
                });
            }
        }));
    }

    private static save(datas: Sensors, path: string): void {
        DatabaseService.insert(datas).subscribe(_ => {
            delete datas.rawMpptchg;

            if (!fs.existsSync(path)) {
                fs.writeFileSync(path, 'date,' + Object.keys(datas).join(',') + '\n');
                LogService.log('sensors', 'CSV created', path);
            }

            fs.appendFileSync(path, new Date().toString() + ',' + Object.values(datas).join(',') + '\n');
            LogService.log('sensors', 'CSV saved');
        });
    }
}
