import { Observable, Observer, of } from 'rxjs';
import { LogService } from './log.service';
import { catchError, map, switchMap, tap } from 'rxjs/operators';
import { RadioService } from './radio.service';
import { SensorsService } from './sensors.service';
import { AprsConfigInterface } from '../config/aprs-config.interface';
import { SensorsConfigInterface } from '../config/sensors-config.interface';
import { DatabaseService } from './database.service';
import { EnumVariable } from '../models/variables';
import ChildProcess = require('child_process');

export class AprsService {

    public static alreadyInUse: boolean;

    public static sendAprsBeacon(config: AprsConfigInterface, keepRadioOn: boolean = false): Observable<void> {
        LogService.log('aprs', 'Start sending APRS Beacon');

        if (AprsService.alreadyInUse) {
            LogService.log('aprs', 'Already in use');
            return of(null);
        }

        AprsService.alreadyInUse = true;

        return RadioService.pttOn().pipe(
            switchMap(_ => {
                return new Observable<void>((observer: Observer<void>) => {
                    try {
                        LogService.log('aprs', 'Send APRS beacon');

                        ChildProcess.execFileSync(config.ax25beaconPath + '/ax25beacon', [
                            '-s ' + config.callSrc,
                            '-d ' + (config.callDest ? config.callDest : 'APRS'),
                            '-p ' + (config.path ? config.path : 'WIDE1-1,WIDE2-1'),
                            '-t ' + (config.symbolTable ? config.symbolTable : '/'),
                            '-c ' + (config.symbolCode ? config.symbolCode : '"'),
                            '' + config.lat,
                            '' + config.lng,
                            config.altitude ? '' + config.altitude : '0',
                            config.comment ? config.comment : ''
                        ], {
                            encoding: 'utf8'
                        });
                        observer.next();
                        observer.complete();
                    } catch (e) {
                        observer.error(e);
                    }
                });
            }),
            switchMap(_ => RadioService.pttOff(!keepRadioOn)),
            tap(_ => {
                AprsService.alreadyInUse = false;
                LogService.log('aprs', 'Send APRS Beacon OK');
            }),
            catchError(err => {
                AprsService.alreadyInUse = false;
                LogService.log('aprs', 'Send APRS Beacon KO', err);
                return RadioService.pttOff(!keepRadioOn);
            }),
        );
    }

    public static sendAprsTelemetry(configAprs: AprsConfigInterface, configSensors: SensorsConfigInterface, keepRadioOn: boolean = false): Observable<void> {
        LogService.log('aprs', 'Start sending APRS Telemetry');

        if (AprsService.alreadyInUse) {
            LogService.log('aprs', 'Already in use');
            return of(null);
        }

        AprsService.alreadyInUse = true;

        return SensorsService.getAllAndSave(configSensors).pipe(
            switchMap(telemetry => RadioService.pttOn().pipe(
                switchMap(_ => DatabaseService.readVariable<number>(EnumVariable.SEQ_TELEMETRY).pipe(map(d => +d))),
                switchMap(seq => {
                    return new Observable<void>((observer: Observer<void>) => {
                        if (seq === 0) {
                            try {
                                LogService.log('aprs', 'Send APRS Telemetry Name');

                                ChildProcess.execFileSync(configAprs.ax25beaconPath + '/ax25frame', [
                                    '-s ' + configAprs.callSrc,
                                    '-d ' + (configAprs.callDest ? configAprs.callDest : 'APRS'),
                                    '-p ' + (configAprs.path ? configAprs.path : 'WIDE1-1,WIDE2-1'),
                                    `:${configAprs.callSrc}   :PARM,VBat,ICharge,Temp`
                                ], {
                                    encoding: 'utf8'
                                });
                            } catch (e) {
                                LogService.log('aprs', 'Send APRS Telemetry Name KO', e);
                            }
                            try {
                                LogService.log('aprs', 'Send APRS Telemetry Unit');

                                ChildProcess.execFileSync(configAprs.ax25beaconPath + '/ax25frame', [
                                    '-s ' + configAprs.callSrc,
                                    '-d ' + (configAprs.callDest ? configAprs.callDest : 'APRS'),
                                    '-p ' + (configAprs.path ? configAprs.path : 'WIDE1-1,WIDE2-1'),
                                    `:${configAprs.callSrc}   :UNIT,Volts,Amps,Celcius`
                                ], {
                                    encoding: 'utf8'
                                });
                            } catch (e) {
                                LogService.log('aprs', 'Send APRS Telemetry Unit KO', e);
                            }
                        }

                        try {
                            LogService.log('aprs', 'Send APRS Telemetry');

                            ChildProcess.execFileSync(configAprs.ax25beaconPath + '/ax25frame', [
                                '-s ' + configAprs.callSrc,
                                '-d ' + (configAprs.callDest ? configAprs.callDest : 'APRS'),
                                '-p ' + (configAprs.path ? configAprs.path : 'WIDE1-1,WIDE2-1'),
                                `T#${seq.toString().padStart(2, '0')},${telemetry.voltageBattery},${telemetry.currentCharge},${telemetry.temperatureRtc}`
                            ], {
                                encoding: 'utf8'
                            });

                            DatabaseService.updateVariable(EnumVariable.SEQ_TELEMETRY, (seq + 1) % 100).pipe(
                                catchError(err => {
                                    observer.error(err);
                                    return err;
                                })
                            ).subscribe(_ => {
                                observer.next(null);
                                observer.complete();
                            });
                        } catch (e) {
                            observer.error(e);
                        }
                    });
                }),
                switchMap(_ => RadioService.pttOff(!keepRadioOn)),
                tap(_ => {
                    AprsService.alreadyInUse = false;
                    LogService.log('aprs', 'Send APRS Telemetry OK');
                }),
                catchError(err => {
                    AprsService.alreadyInUse = false;
                    LogService.log('aprs', 'Send APRS Telemetry KO', err);
                    return RadioService.pttOff(!keepRadioOn);
                }),
            ))
        );
    }
}
