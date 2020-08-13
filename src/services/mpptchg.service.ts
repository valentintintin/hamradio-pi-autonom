import SunCalc = require('suncalc');
import { Observable, of, timer } from 'rxjs';
import { catchError, map, retry, switchMap, tap } from 'rxjs/operators';
import { LogService } from './log.service';
import { CommandMpptChd, CommunicationMpptchdService } from './communication-mpptchd.service';
import { ConfigInterface } from '../config/config.interface';
import * as events from 'events';
import { MpptChhdConfigInterface } from '../config/mppt-chhd-config.interface';

export class MpptchgService {

    public static readonly POWER_OFF_VOLT = 11500;
    public static readonly POWER_ON_VOLT = 12500;
    public static readonly NIGHT_LIMIT_VOLT = 11700;

    private static readonly WD_INIT_SECS = 180; // 2 minutes
    private static readonly WD_INIT_NIGHT_SECS = 255; // 4 minutes 15
    private static readonly WD_UPDATE_SECS = 10; // 10 secondes
    private static readonly WD_ALERT_SECS = 60; // 1 minute
    private static readonly WD_PWROFF_SECS = 10; // 10 secondes
    private static readonly WD_PWROFF_NIGHT_SECS_DEFAULT = 3600; // 1 heure

    private static nightTriggered;

    public static events = new events.EventEmitter();

    private static getSunCalcTime(lat: number, lng: number): SunCalcResultInterface {
        const date = new Date();
        const sunDate: SunCalcResultInterface = SunCalc.getTimes(date, lat, lng);
        if (sunDate.dawn.getTime() < date.getTime()) {
            date.setDate(date.getDate() + 1);
            return SunCalc.getTimes(date, lat, lng);
        }
        return sunDate;
    }

    public static stopWatchdog(): Observable<void> {
        return CommunicationMpptchdService.instance.disableWatchdog();
    }

    public static startWatchdog(): Observable<void> {
        return MpptchgService.enableWatchdog(this.WD_PWROFF_SECS, this.WD_INIT_SECS);
    }

    public static startBatteryManager(config: ConfigInterface): Observable<void> {
        const mpptChd: CommunicationMpptchdService = CommunicationMpptchdService.instance;

        LogService.log('mpptChd', 'Started');

        const powerOnVolt = config.mpptChd.powerOnVolt ?? MpptchgService.POWER_ON_VOLT;
        const powerOffVolt = config.mpptChd.powerOffVolt ?? MpptchgService.POWER_OFF_VOLT;

        return mpptChd.send(CommandMpptChd.PWRONV, powerOnVolt).pipe(
            switchMap(_ => mpptChd.send(CommandMpptChd.PWROFFV, powerOffVolt)),
            tap(_ => LogService.log('mpptChd', 'Power values set', powerOffVolt, powerOnVolt)),
            retry(2),
            catchError(err => {
                LogService.log('mpptChd', 'Impossible to set all power values', powerOffVolt, powerOnVolt);
                return of(null);
            }),
            switchMap(_ => this.stopWatchdog()),
            switchMap(_ => timer(0, this.WD_UPDATE_SECS * 1000)),
            switchMap(_ => this.baterryManagerUpdate(config.lat, config.lng, config.mpptChd))
        );
    }

    private static baterryManagerUpdate(lat: number, lng: number, config: MpptChhdConfigInterface): Observable<void> {
        return CommunicationMpptchdService.instance.getStatus().pipe(
            switchMap(status => {
                LogService.log('mpptChd', 'Loop check', status);

                if (status.alertAsserted && config.batteryLowAlert) {
                    LogService.log('mpptChd', 'Alert asserted !');
                    this.events.emit(EventMpptChg.ALERT);
                } else {
                    if (status.nightDetected && config.nightAlert) {
                        if (!this.nightTriggered || !status.watchdogRunning) {
                            LogService.log('mpptChd', 'Night detected');
                            this.nightTriggered = true;
                            this.events.emit(EventMpptChg.NIGHT_DETECTED);

                            const goodHour = this.getSunCalcTime(lat, lng).dawn;
                            let nextWakeUp = new Date();

                            if (status.values && status.values.batteryVoltage >= (config.nightLimitVolt ?? this.NIGHT_LIMIT_VOLT)) {
                                nextWakeUp.setSeconds(nextWakeUp.getSeconds() + (config.nightSleepTimeSeconds ?? this.WD_PWROFF_NIGHT_SECS_DEFAULT));
                                if (nextWakeUp.getTime() > goodHour.getTime()) {
                                    nextWakeUp = goodHour;
                                }
                            } else {
                                nextWakeUp = goodHour;
                            }
                            return MpptchgService.shutdownAndWakeUpAtDate(nextWakeUp, config.nightRunSleepTimeSeconds ?? this.WD_INIT_NIGHT_SECS).pipe(
                                catchError(err => of(null))
                            );
                        } else {
                            LogService.log('mpptChd', 'Night still here');
                        }
                    } else if (config.watchdog) {
                        this.nightTriggered = false;

                        return MpptchgService.startWatchdog().pipe(
                            catchError(err => of(null))
                        );
                    } else {
                        this.nightTriggered = false;
                    }
                    LogService.log('mpptChd', 'Nothing to do');
                }
                return of(null);
            })
        );
    }

    private static enableWatchdog(secondsBeforeWakeUp: number, secondsBeforeShutdown: number): Observable<void> {
        if (secondsBeforeWakeUp < 1) {
            secondsBeforeWakeUp = 1;
        }
        if (secondsBeforeWakeUp > 65535) {
            throw new Error('Impossible to set more than 65535 seconds before run');
        }
        if (secondsBeforeShutdown < 1) {
            secondsBeforeShutdown = 1;
        }
        if (secondsBeforeShutdown > 255) {
            throw new Error('Impossible to set more than 255 seconds before shutdown');
        }

        return CommunicationMpptchdService.instance.enableWatchdog(secondsBeforeWakeUp, secondsBeforeShutdown).pipe(
            map(_ => {
                LogService.log('mpptChd', 'Watchdog enabled', secondsBeforeWakeUp, secondsBeforeShutdown);
                return null;
            }),
            retry(2),
            catchError(err => {
                LogService.log('mpptChd', 'Watchdog impossible to enabled !', secondsBeforeWakeUp, secondsBeforeShutdown);
                return err;
            })
        );
    }

    public static shutdownAndWakeUpAtDate(wakeUpDate: Date, secondsBeforeShutdown: number = 0): Observable<Date> {
        const now = new Date();

        let wakeUpDateOk = new Date(wakeUpDate);
        if (wakeUpDateOk.getTime() < now.getTime() + 10000) {
            wakeUpDateOk.setSeconds(wakeUpDateOk.getSeconds() + 10);
        }

        let secondsBeforeWakeUp = Math.round((wakeUpDateOk.getTime() - now.getTime()) / 1000) - secondsBeforeShutdown - 60;
        if (secondsBeforeWakeUp < 1) {
            secondsBeforeWakeUp = 1;
        }

        LogService.log('mpptChd', 'Request to shutdown', {
            'now': now,
            'request': wakeUpDate,
            'requestOk': wakeUpDateOk,
            'secondsBeforeWakeUp': secondsBeforeWakeUp,
            'secondsBeforeShutdown': secondsBeforeShutdown
        })

        return this.enableWatchdog(secondsBeforeWakeUp, secondsBeforeShutdown).pipe(
            map(_ => wakeUpDateOk)
        );
    }
}

// https://en.wikipedia.org/wiki/Twilight
interface SunCalcResultInterface {
    sunrise: Date;          // sunrise (top edge of the sun appears on the horizon)
    sunriseEnd: Date;       // sunrise ends (bottom edge of the sun touches the horizon)
    goldenHourEnd: Date;    // morning golden hour (soft light, best time for photography) ends
    solarNoon: Date;        // solar noon (sun is in the highest position)
    goldenHour: Date;       // evening golden hour starts
    sunsetStart: Date;      // sunset starts (bottom edge of the sun touches the horizon)
    sunset: Date;           // sunset (sun disappears below the horizon, evening civil twilight starts)
    dusk: Date;             // dusk (evening nautical twilight starts)
    nauticalDusk: Date;     // nautical dusk (evening astronomical twilight starts)
    night: Date;            // night starts (dark enough for astronomical observations)
    nadir: Date;            // nadir (darkest moment of the night, sun is in the lowest position)
    nightEnd: Date;         // night ends (morning astronomical twilight starts)
    nauticalDawn: Date;     // nautical dawn (morning nautical twilight starts)
    dawn: Date;             // dawn (morning nautical twilight ends, morning civil twilight starts)
}

export enum EventMpptChg {
    NIGHT_DETECTED = 'NIGHT_DETECTED',
    ALERT = 'ALERT'
}
