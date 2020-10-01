import SunCalc = require('suncalc');
import fs = require('fs');
import { Observable, of, timer } from 'rxjs';
import { catchError, map, switchMap, tap } from 'rxjs/operators';
import { LogService } from './log.service';
import { CommandMpptChd, CommunicationMpptchdService } from './communication-mpptchd.service';
import { ConfigInterface } from '../config/config.interface';
import * as events from 'events';
import { MpptChhdConfigInterface } from '../config/mppt-chhd-config.interface';

export class MpptchgService {

    private static readonly POWER_OFF_VOLT = 11500;
    private static readonly POWER_ON_VOLT = 12500;
    private static readonly NIGHT_LIMIT_VOLT = 11600;
    private static readonly PAUSE_BEFORE_POWER_OFF_REACHED_VOLT = 11600;

    private static readonly WD_INIT_SECS = 180; // 2 minutes
    private static readonly WD_INIT_NIGHT_SECS = 255; // 4 minutes 15
    private static readonly WD_UPDATE_SECS = 10; // 10 seconds
    private static readonly WD_PWROFF_SECS = 10; // 10 seconds
    private static readonly WD_PWROFF_NIGHT_SECS_DEFAULT = 3600; // 1 hour
    private static readonly WD_PAUSE_BEFORE_POWEROFF_REACHED_SEC = 1800; // 30 minutes
    private static readonly WD_ALERT_SEC = 60;

    private static nightTriggered: boolean;
    private static lowVoltageTriggered: boolean;

    public static events = new events.EventEmitter();
    public static externalShutdownTriggered: Date;

    private static getSunCalcTime(lat: number, lng: number, tomorrow: boolean): SunCalcResultInterface {
        const date = new Date();
        if (tomorrow) {
            date.setDate(date.getDate() + 1);
        }
        return SunCalc.getTimes(date, lat, lng);
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
            catchError(_ => {
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
                const now = new Date();

                if (status.alertAsserted && config.batteryLowAlert) {
                    LogService.log('mpptChd', 'Alert asserted !');
                    this.events.emit(EventMpptChg.ALERT);
                } else if (!MpptchgService.externalShutdownTriggered || MpptchgService.externalShutdownTriggered.getTime() < now.getTime()) {
                    MpptchgService.externalShutdownTriggered = null;

                    const nextWakeUpLowBattery = new Date();
                    const batteryVoltage = status.values ? status.values.batteryVoltage : Number.MAX_SAFE_INTEGER;
                    const isLowVoltage = batteryVoltage < (config.pauseBeforePowerOffReachedVolt ?? MpptchgService.PAUSE_BEFORE_POWER_OFF_REACHED_VOLT);

                    if (status.nightDetected && config.nightAlert) {
                        if (!this.nightTriggered) {
                            let nextWakeUpNight = new Date();

                            LogService.log('mpptChd', 'Night detected');
                            this.nightTriggered = true;
                            this.events.emit(EventMpptChg.NIGHT_DETECTED);

                            const sunDate = this.getSunCalcTime(lat, lng, true);

                            LogService.log('mpptChd', 'Night status', {
                                now: now,
                                dawn: sunDate.dawn,
                                sunrise: sunDate.sunrise,
                                batteryVoltage
                            });

                            if (!isLowVoltage && batteryVoltage >= (config.nightLimitVolt ?? this.NIGHT_LIMIT_VOLT)) {
                                nextWakeUpNight.setSeconds(nextWakeUpNight.getSeconds() + (config.nightSleepTimeSeconds ?? this.WD_PWROFF_NIGHT_SECS_DEFAULT));
                                if (nextWakeUpNight.getTime() > sunDate.dawn.getTime()) {
                                    LogService.log('mpptChd', 'Morning is too soon, no sleep', sunDate.dawn);
                                    return of(null);
                                }
                            } else {
                                LogService.log('mpptChd', 'Not enough battery so sleep to sunrise', sunDate.sunrise);
                                nextWakeUpNight = sunDate.sunrise;
                            }

                            return MpptchgService.shutdownAndWakeUpAtDate(nextWakeUpNight, config.nightRunSleepTimeSeconds ?? this.WD_INIT_NIGHT_SECS).pipe(
                                catchError(_ => of(null))
                            );
                        } else {
                            LogService.log('mpptChd', 'Night still here', status.values?.watchdogCounter);
                        }
                    } else if (isLowVoltage) {
                        if (!this.lowVoltageTriggered) {
                            this.lowVoltageTriggered = true;

                            nextWakeUpLowBattery.setSeconds(nextWakeUpLowBattery.getSeconds() + (config.pauseBeforePowerOffReachedSeconds ?? MpptchgService.WD_PAUSE_BEFORE_POWEROFF_REACHED_SEC));

                            LogService.log('mpptChd', 'Low battery voltage detected', {
                                batteryVoltage: status.values.batteryVoltage,
                                solarVoltage: status.values.solarVoltage,
                                nextWakeUpLowBattery
                            });

                            return MpptchgService.shutdownAndWakeUpAtDate(nextWakeUpLowBattery, this.WD_INIT_NIGHT_SECS).pipe(
                                catchError(_ => of(null))
                            );
                        } else {
                            LogService.log('mpptChd', 'Still low voltage battery', status.values?.watchdogCounter);
                        }
                    } else if (config.watchdog) {
                        this.nightTriggered = false;
                        this.lowVoltageTriggered = false;

                        return MpptchgService.startWatchdog().pipe(
                            catchError(_ => of(null))
                        );
                    } else {
                        this.nightTriggered = false;
                        this.lowVoltageTriggered = false;
                    }
                    LogService.log('mpptChd', 'Nothing to do');
                } else {
                    LogService.log('mpptChd', 'Nothing to do, waiting for user shutdown', MpptchgService.externalShutdownTriggered);
                }
                return of(null);
            })
        );
    }

    private static enableWatchdog(secondsBeforeWakeUp: number, secondsBeforeShutdown: number): Observable<void> {
        if (secondsBeforeWakeUp < 5) {
            secondsBeforeWakeUp = 5; // enough time for Pi to empty capacitor
        } else if (secondsBeforeWakeUp > 65535) {
            LogService.log('mpptChg', 'Watchdog secondsBeforeWakeUp too much', secondsBeforeWakeUp);
            secondsBeforeWakeUp = 65535;
        }

        if (secondsBeforeShutdown < 5) {
            secondsBeforeShutdown = 5;
        } else if (secondsBeforeShutdown > 255) {
            LogService.log('mpptChg', 'Watchdog secondsBeforeShutdown too much', secondsBeforeShutdown);
            secondsBeforeWakeUp = 255;
        }

        const now = new Date();
        const dataLog = {
            now: now,
            sleepAt: new Date(now.getTime() + (secondsBeforeShutdown + MpptchgService.WD_ALERT_SEC) * 1000),
            wakeupAt: new Date(now.getTime() + (secondsBeforeWakeUp + secondsBeforeShutdown + MpptchgService.WD_ALERT_SEC) * 1000),
            secondsBeforeShutdown: secondsBeforeShutdown + MpptchgService.WD_ALERT_SEC,
            secondsBeforeWakeUp
        };

        fs.writeFileSync('/tmp/watchdog.log', JSON.stringify(dataLog))

        return CommunicationMpptchdService.instance.enableWatchdog(secondsBeforeWakeUp, secondsBeforeShutdown).pipe(
            map(_ => {
                LogService.log('mpptChd', 'Watchdog enabled', dataLog);
                return null;
            }),
            catchError(err => {
                LogService.log('mpptChd', 'Watchdog impossible to enabled !', dataLog);
                return err;
            })
        );
    }

    public static shutdownAndWakeUpAtDate(wakeUpDate: Date, secondsBeforeShutdown: number = 0): Observable<Date> {
        return this.enableWatchdog(
            Math.round((wakeUpDate.getTime() - new Date().getTime()) / 1000) - secondsBeforeShutdown - MpptchgService.WD_ALERT_SEC,
            secondsBeforeShutdown
        ).pipe(
            map(_ => wakeUpDate)
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
