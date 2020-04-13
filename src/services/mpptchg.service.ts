import SunCalc = require('suncalc');
import { Observable, of, throwError, timer } from 'rxjs';
import { catchError, map, retry, switchMap, tap } from 'rxjs/operators';
import { LogService } from './log.service';
import { CommandMpptChd, CommunicationMpptchdService } from './communication-mpptchd.service';
import { exec } from 'child_process';
import { debug } from '../index';
import { ConfigInterface } from '../config/config.interface';

const ON_DEATH = require('death');

export class MpptchgService {

    public static readonly POWER_OFF_VOLT = 11500;
    public static readonly POWER_ON_VOLT = 12500;
    public static readonly NIGHT_LIMIT_VOLT = 11700;

    private static readonly WD_INIT_SECS = 180 / (debug ? 10 : 1);
    private static readonly WD_INIT_NIGHT_SECS = 600 / (debug ? 10 : 1);
    private static readonly WD_UPDATE_SECS = 60 / (debug ? 10 : 1);
    private static readonly WD_PWROFF_SECS = 10 / (debug ? 10 : 1);
    private static readonly WD_PWROFF_NIGHT_SECS_DEFAULT = 3600 / (debug ? 100 : 1);

    private static nightTriggered;
    private static wdPowerOffSec;
    private static wdInitSec;

    private static getSunCalcTime(lat: number, lng: number, tomorrow: boolean = false): SunCalcResultInterface {
        const date = new Date();
        if (tomorrow) {
            date.setDate(date.getDate() + 1);
        }
        return SunCalc.getTimes(date, lat, lng);
    }

    public static getWakeupDate(lat: number, lng: number, allowNight: boolean = false, tomorrow: boolean = false): Date {
        let wakeUp = new Date();
        if (tomorrow) {
            wakeUp.setDate(wakeUp.getDate() + 1);
        }
        const sunTimes: SunCalcResultInterface = this.getSunCalcTime(lat, lng, tomorrow);

        if (wakeUp > sunTimes.dawn && wakeUp < sunTimes.dusk) {
            if (wakeUp < sunTimes.goldenHourEnd || wakeUp > sunTimes.goldenHour) {
                wakeUp.setMinutes(wakeUp.getMinutes() + 1);
            } else {
                wakeUp.setMinutes(wakeUp.getMinutes() + 3);
            }
        } else if (!allowNight) {
            wakeUp = sunTimes.dawn;
        } else {
            wakeUp.setMinutes(0);
            wakeUp.setHours(wakeUp.getHours() + 1);
            if (wakeUp > sunTimes.dawn) {
                wakeUp = sunTimes.dawn;
            }
        }

        wakeUp.setSeconds(0);

        if (wakeUp.getTime() < new Date().getTime()) {
            return this.getWakeupDate(lat, lng, allowNight, true);
        }

        return wakeUp;
    }

    public static stop(): Observable<void> {
        return CommunicationMpptchdService.instance.disableWatchdog();
    }

    public static battery(config: ConfigInterface): Observable<void> {
        const mpptChd: CommunicationMpptchdService = CommunicationMpptchdService.instance;

        LogService.log('mpptChd', 'Started');

        const powerOnVolt = config.mpptChd.powerOnVolt ? config.mpptChd.powerOnVolt : MpptchgService.POWER_ON_VOLT;
        const powerOffVolt = config.mpptChd.powerOffVolt ? config.mpptChd.powerOffVolt : MpptchgService.POWER_OFF_VOLT;

        return mpptChd.send(CommandMpptChd.PWRONV, powerOnVolt).pipe(
            switchMap(_ => mpptChd.send(CommandMpptChd.PWROFFV, powerOffVolt)),
            tap(_ => LogService.log('mpptChd', 'Power values set', powerOffVolt, powerOnVolt)),
            retry(2),
            catchError(err => {
                LogService.log('mpptChd', 'Impossible to set all power values !', powerOffVolt, powerOnVolt);
                return throwError(err);
            }),
            switchMap(_ => timer(0, this.WD_UPDATE_SECS * 1000)), // todo : check if pipe here the last one
            switchMap(_ => this.baterryManagerUpdate(config.lat, config.lng, config.mpptChd.watchdog))
        );
    }

    private static baterryManagerUpdate(lat: number, lng: number, watchdog: boolean): Observable<void> {
        return (CommunicationMpptchdService.instance).getStatus().pipe(
            switchMap(status => {
                LogService.log('mpptChd', 'Get status',);

                if (status.alertAsserted) {
                    LogService.log('mpptChd', 'Alert ! Halt now');
                    if (!debug) {
                        exec('halt');
                    }
                    process.exit(0);
                }

                let wdShouldReset = watchdog;
                if (status.nightDetected) {
                    if (!this.nightTriggered) {
                        LogService.log('mpptChd', 'Night detected');
                        this.nightTriggered = true;
                        this.wdInitSec = this.WD_INIT_NIGHT_SECS;
                        const nextWakeUp = this.getWakeupDate(lat, lng, status.values && status.values.batteryVoltage >= this.NIGHT_LIMIT_VOLT);
                        const timeNight = Math.round((nextWakeUp.getTime() - new Date().getTime()) / 1000);
                        this.wdPowerOffSec = timeNight > this.WD_UPDATE_SECS + 20 ? timeNight : this.WD_PWROFF_NIGHT_SECS_DEFAULT;
                        this.wdPowerOffSec = 10;
                    } else {
                        LogService.log('mpptChd', 'Night still here');
                        wdShouldReset = false;
                    }
                } else {
                    this.nightTriggered = false;
                    this.wdPowerOffSec = this.WD_PWROFF_SECS;
                    this.wdInitSec = this.WD_INIT_SECS;
                }

                if (wdShouldReset) {
                    return (CommunicationMpptchdService.instance).enableWatchdog(this.wdPowerOffSec, this.wdInitSec).pipe(
                        map(_ => {
                            LogService.log('mpptChd', 'Watchdog enabled', this.wdPowerOffSec, this.wdInitSec);
                            return null;
                        }),
                        retry(2),
                        catchError(err => {
                            LogService.log('mpptChd', 'Watchdog impossible to enabled !', this.wdPowerOffSec, this.wdInitSec);
                            return throwError(err);
                        })
                    );
                }
                LogService.log('mpptChd', 'Nothing to do');
                return of(null);
            })
        );
    }
}

interface SunCalcResultInterface {
    sunrise: Date;
    sunriseEnd: Date;
    goldenHourEnd: Date;
    solarNoon: Date;
    goldenHour: Date;
    sunsetStart: Date;
    sunset: Date;
    dusk: Date;
    nauticalDusk: Date;
    night: Date;
    nadir: Date;
    nightEnd: Date;
    nauticalDawn: Date;
    dawn: Date;
}
