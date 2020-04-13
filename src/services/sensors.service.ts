import { Observable, Observer, of } from 'rxjs';
import { LogService } from './log.service';
import { catchError, map, switchMap, tap } from 'rxjs/operators';
import { CommandMpptChd, CommunicationMpptchdService } from './communication-mpptchd.service';
import { SensorsConfigInterface } from '../config/sensors-config.interface';
import { debug } from '../index';
import fs = require('fs');

const si = require('systeminformation');

export interface SensorsData {
    voltageBatterie?: number;
    voltageSolaire?: number;
    intensiteBatterie?: number;
    intensiteSolaire?: number;
    intensiteCharge?: number;
    temperatureBoite?: number;
    temperatureCpu?: number;
    temperatureRtc?: number;
    uptime?: number;
}

export class SensorsService {

    public static getVoltageBattery(): Observable<number> {
        LogService.log('sensors', 'Get MpptChd battery voltage');
        return CommunicationMpptchdService.instance.receive(CommandMpptChd.VB).pipe(catchError(_ => of(-1)));
    }

    public static getVoltageSolar(): Observable<number> {
        LogService.log('sensors', 'Get MpptChd solar voltage');
        return CommunicationMpptchdService.instance.receive(CommandMpptChd.VS).pipe(catchError(_ => of(-1)));
    }

    public static getCurrentBattery(): Observable<number> {
        LogService.log('sensors', 'Get MpptChd battery current');
        return CommunicationMpptchdService.instance.receive(CommandMpptChd.IB).pipe(catchError(_ => of(-1)));
    }

    public static getCurrentSolar(): Observable<number> {
        LogService.log('sensors', 'Get MpptChd solar current');
        return CommunicationMpptchdService.instance.receive(CommandMpptChd.IS).pipe(catchError(_ => of(-1)));
    }

    public static getCurrentCharge(): Observable<number> {
        LogService.log('sensors', 'Get MpptChd charge current');
        return CommunicationMpptchdService.instance.receive(CommandMpptChd.IC).pipe(catchError(_ => of(-1)));
    }

    public static getTemperatureBox(): Observable<number> {
        LogService.log('sensors', 'Get MpptChd temperature');
        return CommunicationMpptchdService.instance.receive(CommandMpptChd.IT).pipe(catchError(_ => of(-1)));
    }

    public static getMpptChgData(): Observable<SensorsData> {
        LogService.log('sensors', 'Get all');

        return SensorsService.getVoltageBattery().pipe(map(data => {
                return {
                    voltageBatterie: data
                } as SensorsData;
            }),
            switchMap(datas => SensorsService.getVoltageSolar().pipe(map(data => {
                datas.voltageSolaire = data;
                return datas;
            }))),
            switchMap(datas => SensorsService.getCurrentBattery().pipe(map(data => {
                datas.intensiteBatterie = data;
                return datas;
            }))),
            switchMap(datas => SensorsService.getCurrentSolar().pipe(map(data => {
                datas.intensiteSolaire = data;
                return datas;
            }))),
            switchMap(datas => SensorsService.getCurrentCharge().pipe(map(data => {
                datas.intensiteSolaire = data;
                return datas;
            }))),
            switchMap(datas => SensorsService.getTemperatureBox().pipe(map(data => {
                datas.temperatureBoite = data;
                return datas;
            })))
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

    public static getAll(): Observable<SensorsData> {
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

    public static getAllAndSave(config: SensorsConfigInterface): Observable<SensorsData> {
        return this.getAll().pipe(
            tap(data => this.save(data, config.csvPath))
        )
    }

    private static save(datas: SensorsData, path: string): void {
        if (!fs.existsSync(path)) {
            fs.writeFileSync(path, 'date,' + Object.keys(datas).join(',') + '\n');
            LogService.log('sensors', 'CSV created', path);
        }

        fs.appendFileSync(path, new Date().toLocaleString() + ',' + Object.values(datas).join(',') + '\n');
        LogService.log('sensors', 'CSV saved with datas', path);
    }
}
